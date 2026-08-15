# 04 — TLS, ALPN и h2c-upgrade

Как соединение «понимает», что дальше идёт HTTP/2, и как чистится I/O-ошибка SSL.

---

## 1. Предпосылка: починка SSL I/O (обязательно, фаза 0)

Сейчас `__read` (`httpserverhandlers.c:111`) и `__wr`
(`http_write_filter.c:134`) после отрицательного возврата из
`openssl_read`/`openssl_write` проверяют **только `errno == EAGAIN/EWOULDBLOCK`**.
Это **неправильно для SSL**: `SSL_read`/`SSL_write` возвращают −1, а реальная
причина — через `SSL_get_error`, и `errno` при `SSL_ERROR_WANT_READ/WRITE`
**не определён**.

### Что нужно
Обернуть разбор ошибки SSL в хелпер:
```c
// src/openssl/openssl.c (новое) или в httpserverhandlers.c
typedef enum { SSL_IO_OK, SSL_IO_WANT_READ, SSL_IO_WANT_WRITE,
               SSL_IO_CLOSED, SSL_IO_ERROR } ssl_io_t;
ssl_io_t openssl_io_status(SSL* ssl, int ret); // вызывает SSL_get_error
```
- `SSL_ERROR_WANT_READ` → re-arm `EPOLLIN` (вернуть «continue»).
- `SSL_ERROR_WANT_WRITE` → re-arm `EPOLLOUT` (это может случиться даже при
  SSL_read на renegotiation; у нас renegotiation запрещён `SSL_OP_NO_RENEGOTIATION`,
  но WANT_WRITE всё ещё возможен на TLS 1.3).
- `SSL_ERROR_ZERO_RETURN` → соединение закрыто корректно.
- `SSL_ERROR_SYSCALL`/`SSL_ERROR_SSL` → ошибка, закрыть.

Выгодно и для HTTP/1.1 (сейчас латентный баг), и **критично** для h2:
мультиплексированные потоки регулярно упираются в WANT_*.

Дополнительно: `SSL_pending()` — drain’ить уже расшифрованные байты без нового
`recv()` (уменьшает число epoll-циклов).

### Тесты
Добавить unit/интеграционные кейсы на WANT_READ/WANT_WRITE (mock OpenSSL через
инжекцию, или через `BIO_s_mem` в тестах). Покрыть в фазе 0.

---

## 2. ALPN — с нуля (фаза 0/2)

### 2.1 Регистрация callback’а
На каждый серверный `SSL_CTX` в `openssl_context_init` (`openssl.c:28-82`):
```c
// Рекламируем оба протокола. Порядок = приоритет сервера: h2 предпочтительнее.
static const unsigned char alpn_protos[] = {
    2, 'h','2',                     // "h2"
    8, 'h','t','t','p','/','1','.','1' // "http/1.1"
};
SSL_CTX_set_alpn_select_cb(ctx, h2_alpn_select_cb, NULL);
```
Либо передать список через `SSL_CTX_set_alpn_protos` для клиента — для сервера
именно select-callback. Callback возвращает выбранный протокол из
предложенных клиентом.

### 2.2 Ветвление после хендшейка
В `__handshake` (`httpserverhandlers.c:661-703`) после `result == 1`
(сейчас сразу `set_http(connection)` на строке 682):
```c
const unsigned char* alpn = NULL; unsigned int alpn_len = 0;
SSL_get0_alpn_selected(connection->ssl, &alpn, &alpn_len);
if (alpn && alpn_len == 2 && memcmp(alpn, "h2", 2) == 0) {
    set_http2(connection);          // новый — см. ниже
} else {
    set_http(connection);           // h1.1 как раньше
}
```
SNI уже отработал раньше (`__sni_callback`) и подменил `ssl_ctx`/`server` —
значит ALPN callback видит уже нужный vhost и может per-host решать, предлагать
ли h2 (например, флаг в конфиге — см. §4).

### 2.3 `set_http2(connection)` — зеркало `set_http`
```c
// protocols/http2/server/h2serverhandlers.c (новое)
int set_http2(connection_t* connection) {
    connection_server_ctx_t* ctx = connection->ctx;
    h2session_t* s = h2session_create(connection, ctx->server);
    if (!s) return 0;
    ctx->parser = s;                       // base.free = h2session_free
    connection->read  = h2_server_guard_read;
    connection->write = h2_server_guard_write;
    // (свой read_buf аллоцирован внутри h2session_create)
    h2_send_initial_settings(s);           // сразу шлём SETTINGS + наш preface
    h2_arm(connection, MPXIN|MPXRDHUP);    // ждём клиентский preface + SETTINGS
    return 1;
}
```

---

## 3. h2c — HTTP/2 поверх открытого текста (фаза 6)

Два пути по RFC 9113 §3.1–§3.4.

### 3.1 Prior-knowledge (прямое h2c)
Клиент начинает сразу с 24-байтной магии `PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n`.

**Снифф в `__read`** (`httpserverhandlers.c:111`), **до** `httpparser_run`:
```c
if (!ctx->http1_started && bytes_read >= 24 &&
    memcmp(connection->buffer, H2_PREFACE_MAGIC, 24) == 0) {
    // переключаемся; 24 байта «съедаются» h2session как preface
    return set_http2_consume_preface(connection, connection->buffer, bytes_read);
}
```
Хранить «мы уже начали парсить h1.1» в маленьком флаге на ctx (или в
`connection->buffer` позиции), чтобы сниффить только первый блок. После первого
непустого не-preface блока — обычный h1.1.

> ⚠️ Нельзя сниффить по `MSG_PEEK` в `__set_protocol` (`multiplexingserver.c:245`)
> — лишний syscall на каждое соединение. Байты уже лежат в `connection->buffer`.

> ✅ **Как сделано (фаза 6).** Набросок выше верен, но `bytes_read >= 24` —
> недостаточное условие: preface может приехать разорванным между TCP-сегментами,
> и обрывок уходил в h1.1-парсер (→ 400). В `__h2c_sniff_preface()` байты
> накапливаются в начале `connection->buffer` (`ctx->h2c_peeked`, < 24, 5 бит),
> следующее чтение дописывается за ними; решение принимается на полных 24 байтах,
> и победивший протокол получает весь префикс. Короткий префикс не разрешается
> раньше времени: `P`/`PR` — это ещё и `POST`/`PUT`/`PATCH`/`PROPFIND`.
>
> Предупреждение про `MSG_PEEK` подтвердилось, но по более важной причине, чем
> лишний syscall: epoll здесь **level-triggered**, и непрочитанные байты
> заставляют EPOLLIN срабатывать снова и снова — клиент, замерший посреди
> preface, крутил бы воркер на 100% CPU. Поэтому снифф именно **потребляет**
> байты, а не подглядывает.

### 3.2 Upgrade через HTTP/1.1 (`Upgrade: h2c`)
Клиент присылает обычный HTTP/1.1 запрос с:
```
Connection: Upgrade, HTTP2-Settings
Upgrade: h2c
HTTP2-Settings: <base64url SETTINGS payload>
```
Сервер отвечает `101 Switching Protocols`, затем **передаёт управление h2**.

**Механизм уже есть** — `switch_to_protocol_t` (`connection_s.h:31-35,48`),
исполняется в `connection_after_write` (`connection_s.c:169-177`) после записи
101. По образцу `switch_to_websockets` (`websocketsswitch.c`):
```c
// в HTTP/1.1-хендлере upgrade-маршрута:
conn_ctx->switch_to_protocol.fn   = set_http2_from_upgrade;
conn_ctx->switch_to_protocol.data = settings_blob;  // из HTTP2-Settings
conn_ctx->switch_to_protocol.data_free = free;
// ответ 101 + Connection: Upgrade + Upgrade: h2c
```
`set_http2_from_upgrade` декодирует `HTTP2-Settings` (base64url), применяет как
peer-SETTINGS, ack’ает, создаёт `h2session` и переводит первый (уже
отправленный) HTTP-запрос в **stream 1** как уже открытый поток (его ответ уже
ушёл в составе 101? — нет, ответ на upgrade-запрос идёт уже как h2-фреймы на
stream 1; см. RFC 9113 §3.2 — запрос «преобразуется» в stream 1).

> ✅ **Как сделано (фаза 6).** Реализовано как `h2_server_set_http2_upgrade()`;
> точка входа для приложения — `h2c_upgrade()` (middleware
> `middleware_h2c_upgrade`). Два уточнения против наброска:
>
> 1. **`HTTP2-Settings` не заменяет client preface.** По §3.2 клиент шлёт
>    полный preface (магия + свой SETTINGS) сразу по получении 101 — заголовок
>    лишь front-load'ит значения настроек, пока 101 ещё в пути. Сессия для
>    Upgrade строится с тем же ожиданием preface, что и остальные два входа;
>    иначе эти 24 байта разбираются как фрейм → `GOAWAY(PROTOCOL_ERROR)`.
>    `nghttp` этот баг не показывает — он закрывает соединение сразу после
>    ответа, не дожидаясь следующего фрейма.
> 2. **Кто владеет запросом.** `__ctx_reset` освобождал `ctx->request` до
>    вызова switch-колбэка. Теперь при взведённом `switch_to_protocol` запрос
>    остаётся жив и достаётся stream 1 (websocket-переключатели освобождают его
>    сами). HPACK-декодирование при этом не нужно: заголовки уже разобраны
>    h1.1-парсером, тело уже прочитано — POST через Upgrade работает.

> Prior-knowledge проще и предпочтительнее для backend-to-backend; upgrade нужен
> для совместимости с клиентами, которые начинают по h1.1. Можно реализовать
> только prior-knowledge в первой итерации h2c.

---

## 4. Конфигурация (расширение `config.json`)

В секцию сервера (`servers[].tls`) и общий блок — флаги управления:
```json
{
  "servers": [{
    "tls": { "fullchain": "...", "private": "...", "ciphers": "..." },
    "http2": {
      "enabled": true,                 // default true при наличии tls
      "h2c": true,                     // default false (явное согласие)
      "max_concurrent_streams": 128,
      "initial_window_size": 65535,
      "max_frame_size": 16384,
      "max_header_list_size": 262144,
      "idle_timeout_seconds": 120,
      "ping_interval_seconds": 30
    }
  }]
}
```
Парсится в `moduleloader.c` (по аналогии с `tls`) → поле в `server_t`.

### TLS 1.3 ciphers (попутная починка) — ✅ СДЕЛАНО
`SSL_CTX_set_cipher_list` настраивает только TLS 1.2 и ниже, поэтому 1.3-suites
из `config.json` молча игнорировались — при том, что формат «1.3 и 1.2 в одной
строке» уже был описан в `frontend/docs/en/ssl-certs.md` и лежал в шаблонном
`config.json`. Теперь `openssl_split_ciphers()` разбирает строку на две группы
(1.3 — токены с префиксом `TLS_`) и отдаёт каждую своему API:
`SSL_CTX_set_cipher_list` и `SSL_CTX_set_ciphersuites`.

Пустая группа означает «оставить дефолты OpenSSL для этой версии»: конфиг с
одними 1.2-именами не должен выключать TLS 1.3, и наоборот. Установить пустой
список нельзя — это отключило бы версию целиком.

---

## 5. Чекпоинт фазы 0

- [ ] `openssl_io_status` внедрён; `__read`/`__wr` используют `SSL_get_error`.
- [ ] ALPN callback зарегистрирован; `SSL_get0_alpn_selected` ветвится в
  `__handshake`.
- [ ] `timerfd`-инфраструктура в epoll воркера (см. `06`).
- [ ] HTTP/1.1 регрессий нет (прогнать существующие тесты + `curl https://...`).

После фазы 0 — `set_http2` пока заглушка, которая отвечает GOAWAY(NO_ERROR); это
позволяет проверить, что ALPN договаривается (видно в `openssl s_client -alpn h2`).
