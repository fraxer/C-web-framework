---
outline: deep
description: HTTP/2 в C Web Framework. Мультиплексирование, h2c upgrade, трейлеры, 103 Early Hints, WebSocket-over-h2, настройка и проверка.
---

# HTTP/2

C Web Framework поддерживает HTTP/2 (RFC 9113) — вторую версию протокола HTTP с мультиплексированием запросов, бинарным фреймингом, сжатием заголовков HPACK и управлением потоком.

::: tip Кратко
HTTP/2 включается **автоматически** для любого сервера с настроенным TLS — через согласование ALPN. Отдельный флаг или секция конфигурации не требуются. Клиенты, не поддерживающие h2, прозрачно получают HTTP/1.1.
:::

## Как это работает

В рамках TLS-рукопожатия сервер через [ALPN](https://datatracker.ietf.org/doc/html/rfc7301) сообщает о поддержке `h2` и `http/1.1`, отдавая предпочтение `h2`. Если клиент тоже предлагает `h2` — соединение переходит в режим HTTP/2, иначе остается на HTTP/1.1. Согласование ALPN происходит **после** выбора виртуального хоста по SNI, поэтому HTTP/2 доступен индивидуально для каждого сервера.

```json
{
    "servers": {
        "s1": {
            "domains": ["example.com"],
            "ip": "0.0.0.0",
            "port": 443,
            "tls": {
                "fullchain": "/etc/ssl/certs/fullchain.pem",
                "private": "/etc/ssl/private/privkey.pem",
                "ciphers": "TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256 ECDHE-RSA-AES256-GCM-SHA384"
            },
            "http": {
                "routes": { "/": { "GET": { "file": "...", "function": "index" } } }
            }
        }
    }
}
```

Этого достаточно: при наличии секции `tls` сервер начинает принимать h2-соединения. Дополнительная конфигурация нужна только для [настройки поведения](#настройка) и для [h2c](#h2c-через-plaintext).

## h2c через plaintext

HTTP/2 без TLS (h2c) поддерживается в двух вариантах и включается явно — сервер не переключается на h2c автоматически:

**1. Upgrade (RFC 9113 §3.2)** — запрос HTTP/1.1 с заголовками `Upgrade: h2c` и `HTTP2-Settings` получает ответ `101 Switching Protocols`, после чего соединение переходит в режим h2. Реализован как готовый middleware `middleware_h2c_upgrade`:

```json
{
    "http": {
        "middlewares": ["middleware_h2c_upgrade"],
        "routes": { ... }
    }
}
```

```c
int middleware_h2c_upgrade(httpctx_t* ctx);
// вернёт 0 — если запрос был обновлён (101 отправлен, цепочка остановлена)
// вернёт 1 — для обычного запроса (выполнение продолжается до обработчика)
```

**2. Prior-knowledge (RFC 9113 §3.4)** — клиент сразу отправляет 24-байтный magic preface `PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n`. Сервер автоматически распознаёт такую сигнатуру на plaintext-соединении и открывает h2-сессию без рукопожатия Upgrade.

::: warning Только plaintext
h2c имеет смысл исключительно для серверов без TLS. Для HTTPS-серверов работает обычный h2 через ALPN — middleware `h2c_upgrade` там не нужен.
:::

## Возможности

### Мультиплексирование

Несколько одновременных запросов в одном TCP-соединении. Каждый запрос — отдельный поток (stream); обработчики разных потоков выполняются параллельно. Сервер анонсирует до **100** конкурентных потоков на соединение (`MAX_CONCURRENT_STREAMS`), избыточные потоки получают `RST_STREAM`/`REFUSED_STREAM`.

### Управление потоком

Двухуровневый flow control (соединение + поток), окно по умолчанию 65535 байт. Окно приёма автоматически масштабируется под пропускную способность (RTT берётся из `TCP_INFO`) вплоть до `http2_recv_window_max`. Это исключает зависание передачи больших тел при узком начальном окне.

### HPACK

Сжатие заголовков через статическую и динамическую таблицы HPACK с кодированием Хаффмана. Поля валидируются строго по tchar-правилам — строже буквы RFC, как в nghttp2.

### Трейлеры

Заголовки, отправляемые **после** тела ответа (например `grpc-status` для gRPC), через HEADERS-фрейм с `END_STREAM`:

```c
int handler_with_trailers(httpctx_t* ctx) {
    httpresponse_t* res = ctx->response;

    res->add_header(res, "Content-Type", "application/grpc");
    send_data(ctx, "...payload...");

    /* Трейлер отправится после тела как отдельный HEADERS-фрейм */
    res->add_trailer(res, "grpc-status", "0");
    res->add_trailer(res, "grpc-message", "OK");

    return 1;
}
```

::: warning Только HTTP/2
Трейлеры работают только по h2. По HTTP/1.1 `add_trailer()` вернёт 0 и запишет предупреждение в лог — chunked-кодирование с заголовком `Trailer` не поддерживается.
:::

### Early Hints (103)

Промежуточный ответ `103 Early Hints` (RFC 8297) позволяет браузеру начать предзагрузку ресурсов, пока обработчик ещё формирует основной ответ. Это рекомендуемая замена Server Push:

```c
int handler_with_hints(httpctx_t* ctx) {
    httpresponse_t* res = ctx->response;

    /* Подсказки отправляются ДО основного ответа */
    res->add_early_hint(res, "Link", "</style.css>; rel=preload; as=style");
    res->add_early_hint(res, "Link", "</app.js>; rel=preload; as=script");
    res->send_early_hints(res);   /* → клиент получает 103 */

    /* ... обработчик работает ... */

    send_data(ctx, "<!doctype html>...");   /* → финальный ответ */
    return 1;
}
```

`send_early_hints()` можно вызывать несколько раз; любой вызов после начала финального ответа игнорируется (1xx обязан предшествовать основному ответу).

### WebSocket поверх HTTP/2

[Extended CONNECT](https://datatracker.ietf.org/doc/html/rfc8441) (RFC 8441) — WebSocket-сессия внутри h2-потока вместо отдельного соединения. Клиент инициирует `:method: CONNECT` с `:protocol: websocket`. Обработчики WebSocket остаются неизменными: туннель прозрачен для прикладного кода, работают broadcasting и `permessage-deflate`.

Чтобы vhost принимал такие соединения, у него должна быть включена секция `websockets` — иначе Extended CONNECT получит `501 Not Implemented`.

## Настройка

Поведенческие параметры HTTP/2 — это переменные окружения из секции `main.env` в `config.json`. Читаются один раз при запуске; значения по умолчанию подобраны для типичной нагрузки.

### Жизненный цикл и поток

| Параметр | По умолчанию | Описание |
|----------|--------------|----------|
| `http2_idle_timeout_sec` | `120` | Таймаут простоя соединения (сек). `0` — отключить |
| `http2_ping_interval_sec` | `0` | Отправлять PING после N сек тишины. `0` — контроль отключён |
| `http2_ping_ack_timeout_sec` | `min(interval, 15)` | Ожидание ACK на PING, после которого соединение закрывается |
| `http2_settings_ack_timeout_sec` | `10` | Таймаут подтверждения SETTINGS (§6.5.3). `0` — отключить |
| `http2_recv_window_initial` | `65535` | Начальный размер окна приёма |
| `http2_recv_window_max` | `4194304` | Потолок авторасширения окна (4 МБ). Равно `initial` — отключить масштабирование |
| `http2_write_quantum` | `65536` | Сколько байт поток отдаёт за раунд, прежде чем уступить сокет (мин. 1024) |

### Защита от злоупотреблений (DoS)

| Параметр | По умолчанию | Описание |
|----------|--------------|----------|
| `http2_max_header_list_size` | `32768` | Лимит размера блока заголовков. `0` — отключить |
| `http2_max_continuation_frames` | `64` | Лимит CONTINUATION-фреймов на блок. `0` — без лимита |
| `http2_abort_rate` | `100` | Бюджет Rapid Reset (RST/сек, CVE-2023-44487). `0` — отключить |
| `http2_abort_burst` | `200` | Пиковый размер бакета Rapid Reset |

Мягкое превышение `http2_max_header_list_size` даёт `431 Request Header Fields Too Large` (соединение выживает); жёсткое (×8) — `GOAWAY(ENHANCE_YOUR_CALM)`. Исчерпание бакета Rapid Reset также закрывает соединение через `GOAWAY`. Точные счётчики злоупотреблений доступны в секции `http2_abuse` маршрута `/metrics`.

Пример настройки:

```json
{
    "main": {
        "env": {
            "http2_idle_timeout_sec": 60,
            "http2_ping_interval_sec": 30,
            "http2_max_header_list_size": 16384
        }
    }
}
```

## Ограничения

| Возможность | Статус | Комментарий |
|-------------|--------|-------------|
| **Server Push** | Убран по решению | Был реализован и прошёл `h2spec`, затем удалён: Chrome убрал push в 106, Firefox/Safari по умолчанию выключен. Замена — [103 Early Hints](#early-hints-103) |
| **Plain CONNECT** | Отклонён | Запрещён намеренно, чтобы сервер не стал открытым прокси. Не путать с Extended CONNECT для WebSocket |
| **HTTP/2-клиент** | Нет | Только серверная роль. HTTP-клиент фреймворка работает по HTTP/1.1 |
| **Трейлеры / 103 по HTTP/1.1** | Нет | Эти возможности существуют только в h2 |
| **PRIORITY** | Игнорируется | `SETTINGS_NO_RFC7540_PRIORITIES = 1`; приоритеты устарели в RFC 9113 |

## Проверка

### curl

```bash
# Принудительно HTTP/2 (нужен TLS)
curl -v --http2 https://example.com/

# Проверка согласования ALPN
curl -v https://example.com/ 2>&1 | grep ALPN
# → * ALPN: server accepted h2.
```

### nghttp

```bash
# h2c через Upgrade на plaintext-сервере
nghttp -v http://example.com/

# h2 over TLS
nghttp -v https://example.com/
```

### Соответствие стандарту

Реализация проходит `h2spec` (147 тестов, 0 ошибок по TLS):

```bash
h2spec -S -h example.com -p 443
```
