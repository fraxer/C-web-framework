# 03 — Фреймы, потоки, flow control

Фрейм-уровень + конечный автомат потоков + окна flow control. Образец
возобновляемого парсера — `protocols/websocket/server/parsers/websocketsparser.{c,h}`.

---

## 1. Фрейм (RFC 9113 §4, §6)

Все кадры после preface имеют единый 9-байтный заголовок:

```
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |  Length (24)                    |   Type (8)   |  Flags (8)   |
 +---------------+---------------+---------------+---------------+
 |R (1)| Stream Identifier (31)                                   |
 +---------------+---------------+---------------+---------------+
 |  Payload (Length bytes) ...                                    |
 +---------------------------------------------------------------+
```

- `Length` — длина payload (без 9-байтного заголовка и без учёта padding при
  подсчёте? — padding ВКЛЮЧЁН в Length, поле Pad Length — первый байт payload).
  Максимум `max_frame_size` (default 16384, max 16777215).
- `Type`: DATA(0), HEADERS(1), PRIORITY(2 — deprecated в RFC 9113, игнорируем
  или отвергаем при `PRIORITY`-фрейме? RFC 9113 §6.3 **рекомендует отвергать**
  как ошибку протокола → GOAWAY PROTOCOL_ERROR. Реализуем валидацию: принять и
  проигнорировать ИЛИ отвергнуть — решение зафиксировать).
- Старший бит `Stream Identifier` (`R`) **зарезервирован**, обязан быть 0, иначе
  PROTOCOL_ERROR.
- Флаги зависят от типа: `END_STREAM(0x1)`, `ACK(0x1)` (для SETTINGS/PING),
  `END_HEADERS(0x4)`, `PADDED(0x8)`, `PRIORITY(0x20)`.

### Типы фреймов и обработка
| Type | Stream | Назначение |
|---|---|---|
| DATA (0) | да | тело запроса/ответа; flow-controlled |
| HEADERS (1) | да | заголовки (HPACK-блок); может нести PRIORITY/PADDED |
| PRIORITY (2) | да | deprecated — отвергаем/игнорируем |
| RST_STREAM (3) | да | немедленное закрытие потока (4 байта = error code) |
| SETTINGS (4) | 0 | параметры; ACK — флаг ACK, payload пустой |
| PUSH_PROMISE (5) | да | server push (опционально, фаза 7) |
| PING (6) | 0 | 8 байт opaque; ACK отражает |
| GOAWAY (7) | 0 | последнее завершение соединения |
| WINDOW_UPDATE (8) | да/0 | прирост flow control window (4 байта, но 31-бит) |
| CONTINUATION (9) | да | продолжение HPACK-блока HEADERS |

---

## 2. Connection preface (RFC 9113 §3.4)

- **Клиент** отправляет 24-байтную магию:
  `PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n` — затем сразу SETTINGS-фрейм.
- **Сервер** в ответ шлёт свой SETTINGS. До получения клиентского preface сервер
  молчит (или уже ответил preface своим SETTINGS по ALPN — это разрешено).

Состояние парсера:
```c
H2FRAME_STAGE_PREFACE    // съедаем 24 байта (только клиент→сервер)
H2FRAME_STAGE_HEADER     // 9 байт заголовка
H2FRAME_STAGE_PAYLOAD    // Length байт payload
H2FRAME_STAGE_PADDING    // (если PADDED) лишние pad-байты после payload
```

### Возобновляемый парсер (клон websocketsparser)
```c
typedef struct h2frame_parser {
    h2frame_stage_e stage;
    size_t pos;                   // курсор в текущей стадии
    uint8_t  header[9];           // накопление 9-байтного заголовка
    uint32_t length;              // распарсенный Length
    uint8_t  type, flags;
    uint32_t stream_id;
    uint8_t  pad_length;          // из первого байта payload при PADDED
    bufo_t*  payload;             // накопление payload между recv()
    int      end_stream_seen;
} h2frame_parser_t;

// Возвращает статус I/O-слою (как websocketsparser_run):
typedef enum {
    H2PARSER_CONTINUE,            // нужно больше байт → ждать epoll
    H2PARSER_FRAME_READY,         // фрейм собран → диспетч обработать, вызвать снова
    H2PARSER_BAD_FRAME,           // PROTOCOL_ERROR → GOAWAY
    H2PARSER_OOM,
} h2parser_status_e;
```

`h2frame_parser_feed(parser, read_buf)` потребляет сколько может из
`h2session->read_buf`, возвращает по `FRAME_READY` с готовым фреймом в `parser`.

> В отличие от HTTP/1.1, где байты живут в общем scratch и затираются каждым
> `recv`, h2 **аккумулирует** сырые байты в `h2session->read_buf` (собственный
> `bufo_t`) и парсер движет курсором. Это устраняет гонку интерливинга.

---

## 3. Конечный автомат потока (RFC 9113 §5.1)

```
                 +------+---------------------------------+
                 | idle |  recv HEADERS / send HEADERS    |
                 +------+        (PUSH_PROMISE)           |
            recv/send HEADERS   |  send PUSH_PROMISE
                  v             v
              +------+     +---------------------+
              | open |     | reserved (local)    |
              +------+     +---------------------+
        END_STREAM/rst        |  send HEADERS
            v                 v
   +-----------------+    +----------------------+
   | half-closed (R) |    | half-closed (local)  |
   +-----------------+    +----------------------+
        | END_STREAM/rst      | END_STREAM/rst
        v                     v
              +--------+
              | closed |
              +--------+
```

Таблица переходов (по приходу фрейма на stream-id):
- HEADERS на idle → создай `h2stream_t`, переход в OPEN (или HALF-CLOSED_REMOTE
  если END_STREAM).
- DATA/RST на несуществующем (idle) потоке, кроме stream-id > last_peer →
  PROTOCOL_ERROR.
- Поток с id ≤ goaway_last_stream_id после GOAWAY → REFUSED_STREAM.
- Чётные stream-id от клиента → PROTOCOL_ERROR (клиент не может открывать чётные).
- stream-id ≤ уже виденного от этого пира → PROTOCOL_ERROR (строго возрастают).

`h2stream_state_e` хранится в `h2stream_t.state` (см. `01`).

---

## 4. Flow control (RFC 9113 §6.9)

Два уровня окон: **соединение** и **поток**. Оба начинаются с
`SETTINGS_INITIAL_WINDOW_SIZE` (default 65535, max 2³¹−1).

- Окно **уменьшается** при отправке/приёме DATA на `payload_length` (без pad
  и без 9-байтного заголовка).
- Окно **увеличивается** фреймом WINDOW_UPDATE (`window_size_increment`, 1..2³¹−1).
- Отправка DATA разрешена только если **оба** окна (conn + stream) ≥ размера.
- Окно ≤ 0 → ставим DATA в очередь (`h2session->outbox`), ждём WINDOW_UPDATE.
- Получение DATA: уменьшаем recv-окно; по достижении порога шлём WINDOW_UPDATE
  (порог и размер окна — см. «Авто-масштабирование recv-окна» ниже).
- WINDOW_UPDATE с increment 0 → PROTOCOL_ERROR.
- Переполнение окна (отправка больше окна) → FLOW_CONTROL_ERROR.
- Запрещено flow-control’ить не-DATA фреймы (HEADERS, SETTINGS и т.д.).

### Реализация эмиссии ответа (связка с `05`)
```c
// В h2_write_filter: тело ответа дробится на куски
size_t chunk = min3(peer.max_frame_size,
                    stream->send_window,
                    session->send_window);
if (chunk == 0) → запросить EPOLLOUT и отложить (возобновить по WINDOW_UPDATE);
emit DATA frame(stream_id, chunk байт из response->body по resp_body_offset,
                END_STREAM если добрали до конца);
stream->send_window   -= chunk;
session->send_window  -= chunk;
stream->resp_body_offset += chunk;
```

> ⚠️ **Типичный баг:** забытьEnlarge окно при приёме DATA → large-response
> зависает, т.к. клиент упирается в нулевое recv-окно. Обязательно: при приёме
> DATA увеличиваем своё recv-окно и периодически шлём WINDOW_UPDATE.

### Авто-масштабирование recv-окна (RFC §6.9.1)

Фиксированное окно 65535 ограничивает **входящую** скорость величиной
`окно / RTT` независимо от полосы: 0.6 МБ/с на канале 100 мс, на всё соединение,
сколько бы потоков ни грузило. Поэтому окно приёма растёт к измеренному BDP.

`h2_recv_credit()` (`h2session.c`) обслуживает оба уровня — соединение
(`stream_id = 0`) и поток — одной и той же структурой `h2_recv_window_t`
(`size` / `pending` / `bytes` / `epoch_ms`):

1. Кредит возвращается порогом `max(size/8, 16 КБ)` — при большом окне
   фиксированные 16 КБ означали бы сотни WINDOW_UPDATE на окно.
2. Цель — **два BDP**: один держит пира в отправке, пока наш WINDOW_UPDATE в
   пути, второй покрывает устаревание замера.
   `target = 2 · bytes · rtt / elapsed`.
3. Замер скорости учитывается, только если период ≥ RTT (иначе одиночный
   всплеск читается как бесконечная полоса), и шаг роста ограничен ×2 — шумный
   замер не выбрасывает окно сразу в максимум.
4. Окно только растёт, до `http2_recv_window_max`. На быстром канале
   измеренный BDP не дотягивает до 65535 и окно не двигается вовсе — loopback
   и LAN сохраняют прежний рисунок фреймов.
5. Новый поток открывается с окном, до которого соединение уже доросло
   (`stream_recv_learned`): §6.9.2 разрешает SETTINGS задать только начальное
   значение, поэтому разница выдаётся сразу WINDOW_UPDATE'ом.

**Источник RTT.** `TCP_INFO` (`tcpi_rtt`) — даром, обновляется на каждый ACK и
одинаково работает под TLS, но видит только ближайший TCP-хоп: за TCP-балансиром
это RTT до балансира, а не до клиента. Поэтому дополнительно меряем путь
**своим PING** (§6.7) и берём больший из двух. Tune-PING шлётся только пока окно
ещё может расти, по одному в полёте и не чаще `max(RTT, 50 мс)` — за рампу
выходит по одному PING на круг, после стабилизации окна они прекращаются. Это
отдельная от watchdog'а машинерия (`tune_ping_*` против `ping_*`): tune-PING
только меряет и никогда не объявляет пира мёртвым.

**Что это не меняет.** Только сторону приёма — исходящие ответы ограничены
окном, которое анонсирует клиент. Тело запроса и так спулится в tmp-файл, так
что большое окно не превращается в RAM; риск большого окна — сколько байт пир
успеет прислать до того, как мы его остановим, и он ограничен
`http2_recv_window_max` плюс `client_max_body_size`.

---

## 5. SETTINGS (RFC 9113 §6.5)

- При `set_http2` сервер **сразу** шлёт свой SETTINGS (payload — список
  `id(16)+value(32)`). Никакого ACK на свой исходящий SETTINGS не требуется,
  но клиент пришлёт ACK (флаг ACK, пустой payload) → пометить `peer_settings_acked`.
- Приём peer-SETTINGS: применить (с перепроверкой динамической таблицы HPACK при
  изменении `HEADER_TABLE_SIZE`, и пересчётом окон при изменении
  `INITIAL_WINDOW_SIZE` — это влияет на **все** существующие потоки, RFC §6.9.2),
  ответить ACK. Пересчёт — это **дельта**, а не присваивание, и если она уводит
  окно хоть одного потока за 2³¹−1, это connection error `FLOW_CONTROL_ERROR`
  (проверять результат сложения, а не только значение настройки: переполнение
  достижимо лишь связкой `WINDOW_UPDATE` + SETTINGS — см. 08 §G.1).
- ACK обязан приходить на каждый SETTINGS ровно один раз; SETTINGS с payload в
  ответ на ACK = PROTOCOL_ERROR.
- Незнакомые id игнорируем; in-range значения проверяем на валидность.

---

## 6. CONTINUATION (RFC 9113 §6.10)

HEADERS, чей HPACK-блок не помещается в `max_frame_size`, продолжается цепочкой
CONTINUATION. Правила:
- После HEADERS без `END_HEADERS` **все** последующие фреймы на этом соединении
  обязаны быть CONTINUATION с тем же stream_id, пока не придёт CONTINUATION с
  `END_HEADERS`. Любой другой фрейм между ними → PROTOCOL_ERROR.
- Собираем конкатенацию payload’ов HEADERS(+CONTINUATION*) в один логический
  блок, потом один раз декодируем HPACK.

Состояние «collecting headers for stream N» хранится в `h2session` (это
connection-level, не stream-level):
```c
uint32_t  cont_stream_id;     // 0 = не собираем
bufo_t*   cont_block;         // накопление HPACK-байтов
int       cont_end_headers;
```

> На практике: для типичных заголовков хватает одного HEADERS; но тесты `h2spec`
> обязательно проверяют CONTINUATION — реализовать корректно с первого раза.

---

## 7. RST_STREAM / ошибки

- RST_STREAM(4 байта = error code) → поток в CLOSED, любые ожидающие DATA по
  этому потоку отбрасываются. Ответ, если уже частично отправлен, — добивать не
  нужно.
- Сценарий «клиент закрыл вкладку» → RST на все активные потоки.
- Error codes: NO_ERROR(0), PROTOCOL_ERROR(1), INTERNAL_ERROR(2),
  FLOW_CONTROL_ERROR(3), SETTINGS_TIMEOUT(4), STREAM_CLOSED(5),
  FRAME_SIZE_ERROR(6), REFUSED_STREAM(7), CANCEL(8), COMPRESSION_ERROR(9),
  CONNECT_ERROR(10), ENHANCE_YOUR_CALM(11), INADEQUATE_SECURITY(12),
  HTTP_1_1_REQUIRED(13).

---

## 8. Жёсткие лимиты и валидации (чек-лист против PROTOCOL_ERROR)

- Фрейм на stream_id=0 для типов, требующих поток (HEADERS/DATA/RST/…) →
  PROTOCOL_ERROR.
- Фрейм с потоком ≠ 0 для connection-level типов (SETTINGS/PING/GOAWAY) →
  PROTOCOL_ERROR.
- Length сверх max_frame_size → FRAME_SIZE_ERROR.
- DATA/RST/HEADERS на закрытом потоке → STREAM_CLOSED (RST) или PROTOCOL_ERROR.
- SETTINGS с потоком ≠ 0 → PROTOCOL_ERROR.
- WINDOW_UPDATE increment == 0 → PROTOCOL_ERROR.
- HEADERS с одновременно PRIORITY и отсутствием зависимой инфо → FRAME_SIZE_ERROR.
- Зарезервированный бит R == 1 → PROTOCOL_ERROR.
- Появление `Connection`/`Transfer-Encoding`/`Keep-Alive`/`Upgrade` в
  псевдо/обычных заголовках h2 → PROTOCOL_ERROR (RFC 9113 §8.1.2.2).
- Псевдо-заголовки после обычных → PROTOCOL_ERROR.

Эти проверки — материал для unit-тестов фрейм-парсера в фазе 2.

---

## 9. Диспетчеризация фрейма (скелет)

```c
static int h2_dispatch(h2session_t* s, h2frame_parser_t* f) {
    switch (f->type) {
    case DATA:          return h2_on_data(s, f);
    case HEADERS:       return h2_on_headers(s, f);     // +CONTINUATION
    case RST_STREAM:    return h2_on_rst(s, f);
    case SETTINGS:      return h2_on_settings(s, f);
    case PUSH_PROMISE:  return h2_on_push_promise(s, f); // клиент→сервер запрещён
    case PING:          return h2_on_ping(s, f);
    case GOAWAY:        return h2_on_goaway(s, f);
    case WINDOW_UPDATE: return h2_on_window_update(s, f);
    case CONTINUATION:  return h2_on_continuation(s, f);
    case PRIORITY:      return h2_on_priority(s, f);     // deprecated
    default:            return H2_OK;                     // игнорировать unknown (RFC §5.5)
    }
}
```

`h2_on_headers`/`h2_on_data` строят `httprequest_t` для потока; по
`END_STREAM` (на HEADERS без тела или на финальном DATA) поток уходит в
`__handle` (как в HTTP/1.1, `httpserverhandlers.c:261`) — см. `05`.
