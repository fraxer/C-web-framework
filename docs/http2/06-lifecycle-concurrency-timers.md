# 06 — Жизненный цикл, конкурurentность, таймеры

Самые тонкие темы. Здесь же — teardown и refcount.

---

## 1. Таймеры (сейчас их нет)

В кодовой базе **нет** application-level таймеров: только TCP-keepalive
(`socket.c:30-48`) и 1000ms `epoll_wait` timeout (`multiplexingepoll.c:66`),
который не закрывает простаивающие соединения. Для HTTP/2 нужны:

- **idle timeout** — закрывать соединение без активности (default ~120 с).
- **PING watchdog** — периодически пинговать, если нет исходящего трафика;
  если ACK не пришёл за таймаут → GOAWAY.
- **handshake timeout** — уже частично есть (`__handshake` ждёт epoll);
  добавить верхнюю границу.

### Реализация: `timerfd` в тот же epoll (фаза 0)
Один `timerfd` **на воркер-поток**, зарегистрированный в его epoll, тикающий
каждые N мс (например 500 мс). В обработчике тика — обход соединений воркера и
проверка дедлайнов.

```c
// src/multiplexing/ (расширение mpxapi)
int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK|TFD_CLOEXEC);
timerfd_settime(timerfd, 0, &(it){.it_interval={0,N*1000000}, .it_value={0,N*1000000}}, NULL);
// зарегистрировать timerfd в epoll воркера; его ev.data.ptr указывает на
// worker-level timer-обработчик (НЕ на connection_t).
```

Структура учёта:
```c
typedef struct {
    uint64_t last_activity_ms;   // обновляется при любом read/write фрейма
    uint64_t ping_sent_ms;       // 0 = PING не висит
    uint32_t ping_payload;       // для сверки ACK
} h2_timeouts_t;  // часть h2session_t
```

На тике таймера по списку соединений воркера:
- `now - last_activity_ms > idle_timeout` → GOAWAY(NO_ERROR) + закрытие.
- `ping_interval > 0 && now - last_activity_ms > ping_interval && !ping_in_flight`
  → отправить PING.
- `ping_in_flight && now - ping_sent_ms > ping_ack_timeout` → GOAWAY + закрытие.

> Поддержка подключения таймера к LT-epoll: читаем `timerfd` (8 байт) — он
> level-triggered, поэтому читать обязательно, иначе бесконечные срабатывания.

> ⚠️ PING'ов в сессии **два вида**, и путать их нельзя. Watchdog выше решает,
> жив ли пир. Отдельный tune-PING (`tune_ping_*`) меряет RTT для
> авто-масштабирования recv-окна (`03` §4) — он только измеряет, неотвеченный
> tune-PING никого не закрывает, а лишь перепосылается через 5 с. Сверка ACK
> идёт по обоим payload'ам независимо.

### Альтернатива: min-heap дедлайнов
Если обход всех соединений воркера дорог — min-heap ближайших дедлайнов на
воркер и `timerfd_settime` под следующий. На старте достаточно линейного обхода
(соединений на воркер обычно сотни, не десятки тысяч).

---

## 2. Конкурентность: модель блокировок

Сейчас один спинлок `ctx->locked` (`connection_s.c:114-137`) сериализует всё на
соединении. Для h2 этого недостаточно: N потоков-обработчиков могут
одновременно исполнять хендлеры разных потоков, и все хотят писать фреймы в
один сокет.

### Иерархия блокировок
1. **`session->outbox_lock`** — сериализатор **записи** на соединение. Только
   один поток собирает/пишет батч фреймов за раз. (Образец: `broadcast.c` —
   `__broadcast_queue_add` + spinlock.)
2. **`session->streams_lock`** — защита `hashmap_t* streams` (вставка/удаление/
   поиск потока). Поток после извлечения `h2stream_t*` работает с его полями
   под **потоковой** блокировкой.
3. **`stream->lock`** — защита полей конкретного потока (состояние, курсоры
   эмиссии, recv-окно).
4. **`session->fc_lock`** — connection-level flow control окна (`send_window`,
   `recv_window`),因为他们 могут меняться из многих потоков.

### Порядок захвата (во избежание дедлока)
`outbox_lock → streams_lock → stream->lock` (всегда в этом порядке; никогда не
захватывать `streams_lock`, удерживая `stream->lock`).

### Read-путь (воркер-поток, один на соединение за раз)
- epoll гарантированно вызывает `connection->read` под `ctx->locked` для
  данного соединения; конкурентных ридеров нет.
- Парсит фреймы, мутирует потоки под их блокировками, кладёт готовые запросы в
  `ctx->queue` (очередь уже потокобезопасна).

### Write-путь (много потоков-обработчиков)
- Хендлер заполнил `httpresponse_t`. Вызывает `h2_write_filter_run(stream)`.
- Под `stream->lock` определяет кусок; под `session->fc_lock` резервирует окно;
- Собирает HEADERS/DATA → под `session->outbox_lock` добавляет в outbox и
  выполняет `connection_data_write` (батч).
- Если сокет насыщен (EAGAIN/WANT_WRITE) — выставляет `need_write`, re-arm
  EPOLLOUT; отложенный остаток допишется из `__write` при следующем EPOLLOUT.

### Возобновление по WINDOW_UPDATE
При приходе WINDOW_UPDATE (через read-путь) — поднимаем потоки, чьи
`outbox`-элементы ждут окна, и будим их запись (через флаг/condvar или
простановкой в очередь воркера). Простой вариант: WINDOW_UPDATE форсирует
EPOLLOUT-пробуждение соединения, а `__write` дрейнит все готовые потоки.

---

## 3. Refcount и teardown

`connection_server_ctx_t.ref_count` (atomic) уже управляет временем жизни
соединения (`connection_s_dec`, `connection_s.c:139-157`). Для h2:

- `h2session_t` **не имеет собственного** refcount — его жизнь = жизнь
  `connection`, потому что сессия лежит в `ctx->parser`.
- `h2stream_t` получает **свой** `atomic_int ref`:
  - +1 при диспетче в `ctx->queue` (пока хендлер работает);
  - +1 при отложенной эмиссии ответа (в outbox);
  - освобождается при CLOSED **и** ref==0.
- Это защищает от use-after-free: поток может быть RST’нут клиентом, пока
  хендлер ещё работает — RST переводит состояние в CLOSED, но структура живёт,
  пока хендлер не вернётся и не декрементирует.

### `h2session_free` (вызывается из `__ctx_free`, `connection_s.c:331-353`, через
`parser->base.free`)
- Закрыть все потоки: для каждого `h2stream_t` — освободить `request`,
  `response`, `req_body`, самой структуре.
- `hpack_decoder_free`/`hpack_encoder_free` (dynamic tables).
- `bufo_free(read_buf)`, `bufo_free(write_buf)`, освободить outbox + cont_block.
- `hashmap_free(streams)`.
- Освободить саму `h2session_t`.

### GOAWAY-цикл закрытия
1. Решение закрыть (idle, shutdown, ошибка) → `h2_send_goaway(last_stream_id,
   error)`; `goaway_sent=1`.
2. Новые потоки (id > last) → REFUSED_STREAM.
3. Дать активным потокам завершить ответы.
4. Когда активных потоков 0 → `connection_close`.

При жёстком `shutdown` (SIGTERM) — существующий механизм `signal.c:98-123`
 shutdown() всех сокетов; h2-соединениям при этом желательно сначала послать
 GOAWAY(NO_ERROR), но это best-effort.

---

## 4. Корректность обработки «состояний гонки»

| Сценарий | Реакция |
|---|---|
| DATA пришёл после END_STREAM на потоке | STREAM_CLOSED / RST_STREAM |
| Хендлер отправил ответ, но клиент прислал RST раньше | ответ отбрасывается; поток в CLOSED; ref-- |
| SETTINGS изменил initial_window_size | пересчёт ВСЕХ существующих потоков (RFC §6.9.2): delta = new-old применяется к `stream->send_window` |
| WINDOW_UPDATE на закрытом потоке | игнорировать (или STREAM_CLOSED) |
| Поток с id=0 на потоко-специфичном типе | PROTOCOL_ERROR → GOAWAY |
| PUSH_PROMISE от клиента | PROTOCOL_ERROR (клиент не имеет права) |

Эти сценарии — прямые тест-кейсы (фаза 4/5).

---

## 5. Логи и диагностика

- Использовать существующий `log.h` для пометки h2-событий: ALPN-выбор,
  создание/закрытие потока, GOAWAY-причина, flow-control stall.
- Метрики (опц.): счётчики streams/concurrent, send_window usage, PING-latency —
  счётчиками в `server_t->http2.*`, как у ratelimiter/broadcast.

---

## 6. Чекпоинт фазы 5 (lifecycle) — ✅ выполнено

- [x] idle-timeout работает (простой соединения закрывается GOAWAY(NO_ERROR)).
- [x] PING шлётся и ACK обрабатывается; висячий PING → закрытие.
- [x] GOAWAY корректно завершает соединение, не роняя идущие ответы (дрейн
  активных потоков; новые `id > last_stream_id` → REFUSED_STREAM).
- [x] SIGTERM → GOAWAY + bounded drain, без утечек.
- [x] Нагрузка: 750 000+ потоков, idle-close и RST под нагрузкой — утечек памяти
  нет (ASan + LeakSanitizer в Debug-сборке чист). См. детали в `07`.
