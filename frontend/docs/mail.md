---
outline: deep
description: "Отправка email в C Web Framework. Встроенный SMTP-клиент: прямая доставка на MX-сервер получателя или отправка через SMTP-релей с аутентификацией, DKIM-подписи."
---

# Отправка Email

Фреймворк включает встроенный SMTP-клиент с двумя режимами доставки:

- **прямая доставка** — письмо уходит напрямую на MX-сервер получателя, без внешнего сервиса и без учётных данных. Режим по умолчанию;
- **отправка через релей** — письмо передаётся заданному SMTP-серверу (`smtp.mail.ru`, `smtp.yandex.ru`, корпоративный relay) с логином и паролем.

Режим выбирается **конфигурацией**, а не кодом: появление объекта `mail.relay` в `config.json` переключает доставку на релей. Функции `send_mail()` и `send_mail_async()` и структура `mail_payload_t` в обоих режимах одни и те же — приложение не знает, каким путём ушло письмо.

## Какой режим выбрать

Прямая доставка не требует внешнего сервиса, но требует репутации: без корректных PTR, SPF и DKIM и с «непрогретого» IP письмо уходит в спам или отбивается (`550 spam message rejected`). Это разумный выбор, когда домен и адрес уже с хорошей репутацией.

Релей нужен там, где такой репутации нет и заводить её ради формы обратной связи незачем: репутацией занимается провайдер релея, он же обычно подписывает письмо собственным DKIM.

## Как это работает

### Прямая доставка

1. Из email-адреса получателя извлекается домен и преобразуется в punycode (поддержка IDN).
2. По домену резолвятся MX-записи; клиент подключается к серверу с наивысшим приоритетом на порт **25**.
3. Выполняется `EHLO`, затем `STARTTLS` — TLS поднимается **на том же соединении**, порт не меняется, после чего `EHLO` повторяется поверх TLS.
4. Письмо подписывается DKIM (если ключ задан) и отправляется (`MAIL FROM`, `RCPT TO`, `DATA`).

::: tip
Поскольку доставка идёт напрямую с вашего сервера, для нормальной доставляемости нужны корректные DNS-записи (MX, SPF, DKIM) и «чистый» IP, не внесённый в чёрные списки.
:::

### Через релей

1. Имя релея резолвится через `getaddrinfo()` (A и AAAA — IPv6-релей поддерживается); адреса перебираются до первого успешного соединения на заданный порт. MX-записи не запрашиваются вовсе, существование MX у домена получателя не проверяется.
2. При `security: "tls"` TLS-рукопожатие выполняется сразу после `connect`, **до** чтения баннера; `STARTTLS` не посылается.
3. Выполняется `EHLO`; из многострочного ответа разбирается список расширений (`STARTTLS`, `SIZE`, `AUTH`).
4. При `security: "starttls"` посылается `STARTTLS` — но только если сервер его объявил — и `EHLO` повторяется поверх TLS: список механизмов `AUTH` сервера обычно объявляют лишь после шифрования.
5. Если заданы `user` и `password`, выполняется `AUTH` (см. [Аутентификация](#аутентификация)).
6. Дальше — как при прямой доставке: `MAIL FROM`, `RCPT TO`, `DATA`.

Кодирование содержимого (в обоих режимах):
- **Тема** и **имя отправителя** кодируются как RFC 2047: `=?UTF-8?B?…?=` (поддержка любого Unicode).
- **Тело** кодируется в base64 с переносом строк по 76 символов.
- Заголовки всегда: `Content-Type: text/html; charset=utf-8` и `Content-Transfer-Encoding: base64` — поэтому HTML-письма поддерживаются «из коробки».

## Конфигурация

Настройки почты указываются в файле `config.json`:

```json
{
    "mail": {
        "dkim_private": "/path/to/dkim_private.pem",
        "dkim_selector": "mail",
        "host": "example.com"
    }
}
```

**Параметры:**
- `dkim_private` — путь к приватному RSA-ключу DKIM (PEM). Необязателен: без него письма уходят без подписи.
- `dkim_selector` — селектор DKIM; вместе с `host` образует запись `<selector>._domainkey.<host>`.
- `host` — ваш домен отправителя. Используется в команде `EHLO`, в теге DKIM `d=` и в домене `Message-Id`. Должен совпадать с доменом из DKIM/SPF.

::: warning DKIM — всё или ничего
`dkim_private` и `dkim_selector` задаются **вместе**. Одно без другого — ошибка конфигурации, останавливающая запуск: подписать письмо это не даёт, а молча отправить его без подписи было бы неверным толкованием опечатки. Если не задано ни одно из них, DKIM просто не применяется.
:::

### Секция relay

Наличие объекта `relay` — и есть переключатель режима.

```json
{
    "mail": {
        "host": "example.com",
        "relay": {
            "host": "smtp.mail.ru",
            "port": 587,
            "security": "starttls",
            "user": "info@example.com",
            "password": "...",
            "auth": "auto",
            "timeout": 15,
            "verify": true
        }
    }
}
```

| Поле | Обязательное | По умолчанию | Описание |
|------|--------------|--------------|----------|
| `host` | **да** | — | Имя или адрес сервера-релея |
| `port` | нет | по `security`: 587 / 465 / 25 | Порт |
| `security` | нет | `starttls` | `starttls`, `tls` (implicit TLS), `none` |
| `user` | нет | — | Логин; без него `AUTH` не выполняется |
| `password` | нет | — | Пароль |
| `auth` | нет | `auto` | `auto`, `plain`, `login`, `none` |
| `timeout` | нет | 30 | Таймаут операций сокета, секунды |
| `verify` | нет | `true` | Проверять сертификат и имя хоста релея |

Проверки при загрузке конфигурации:

- `user` без `password` (и наоборот) — ошибка конфигурации, а не молчаливая отправка без `AUTH`;
- `security: "none"` без `user` — валидно: внутренний релей, принимающий почту из локальной сети;
- `security: "none"` вместе с `user` — предупреждение в журнал: учётные данные уйдут открытым текстом. Само по себе это разрешено, но отправлять пароль по нешифрованному каналу клиент согласится **только** при явном `security: "none"`.

Пароль не попадает ни в один журнал, а буферы с ним затираются `explicit_bzero()` сразу после использования.

::: tip Пароль в репозитории
`config.json` удобно собирать из шаблона в entrypoint контейнера, подставляя значения из `.env` — тогда пароль не лежит в репозитории. Отдельного механизма секретов фреймворк не вводит.
:::

### Аутентификация

Поддерживаются два механизма — их предлагают все распространённые релеи:

- **`PLAIN`** (RFC 4616) — одна команда, аргумент `base64("\0" + логин + "\0" + пароль)`;
- **`LOGIN`** — три шага: `AUTH LOGIN`, затем логин и пароль по одному, каждый в base64, каждый в ответ на `334`.

При `auth: "auto"` выбирается `PLAIN`, если сервер его объявил, иначе `LOGIN`. Если задан конкретный механизм, а сервер его не предлагает — отправка прекращается с сообщением о том, что сервер предложил на самом деле; молчаливого перехода на другой механизм не происходит.

`AUTH` выполняется **после** установления TLS. Код `535` (неверные учётные данные) пишется в журнал отдельным сообщением — это самая частая ошибка настройки.

::: tip Gmail
Для `smtp.gmail.com` нужен «пароль приложения», а не пароль аккаунта. `CRAM-MD5` и `XOAUTH2` не поддерживаются.
:::

### TLS и проверка сертификата

При `verify: true` (по умолчанию) для релея загружается системное хранилище доверенных корней, включается `SSL_VERIFY_PEER` и сверяется имя хоста; в SNI передаётся `relay.host` (кроме случая, когда это IP-адрес). Самоподписанный сертификат внутреннего релея — законный повод выставить `"verify": false`.

Для **прямой** доставки сертификат MX-сервера не проверяется, и это намеренно: доставка на чужой MX — оппортунистический TLS (RFC 7435), сертификаты там сплошь и рядом не совпадают с именем MX, и отказ от таких соединений означал бы отказ от доставки.

### Готовые варианты

**Прямая доставка с подписью.** Режим по умолчанию: секции `relay` нет.

```json
"mail": {
    "dkim_private": "/etc/cwfr/dkim_private.pem",
    "dkim_selector": "mail",
    "host": "example.com"
}
```

**Прямая доставка без подписи.** Секцию `mail` можно опустить целиком или оставить один `host` — письма уйдут на MX получателя без `DKIM-Signature`.

```json
"mail": {
    "host": "example.com"
}
```

**Релей на submission (587).** Типовой случай; всё, кроме обязательного `host`, берётся по умолчанию: `port: 587`, `security: "starttls"`, `auth: "auto"`, `timeout: 30`, `verify: true`. DKIM не задан — подписывать будет сам релей, как оно обычно и есть.

```json
"mail": {
    "host": "example.com",
    "relay": {
        "host": "smtp.mail.ru",
        "user": "info@example.com",
        "password": "..."
    }
}
```

**Implicit TLS (465).** `port` подставится сам; рукопожатие выполняется до баннера, `STARTTLS` не посылается.

```json
"relay": {
    "host": "smtp.yandex.ru",
    "security": "tls",
    "user": "info@example.com",
    "password": "..."
}
```

**Явно заданный механизм.** Если сервер предложит только другой — отправка прекратится с сообщением о том, что он предложил на самом деле; молчаливого перехода не будет. Для Gmail нужен пароль приложения.

```json
"relay": {
    "host": "smtp.gmail.com",
    "port": 587,
    "security": "starttls",
    "auth": "login",
    "user": "info@example.com",
    "password": "<app password>"
}
```

**Внутренний релей без аутентификации.** `port` станет 25, `AUTH` не выполняется. Валидная конфигурация, предупреждений нет.

```json
"relay": {
    "host": "postfix.internal",
    "security": "none"
}
```

**Внутренний релей с самоподписанным сертификатом.** Без `"verify": false` такое соединение отвергается.

```json
"relay": {
    "host": "smtp.internal.lan",
    "security": "tls",
    "verify": false,
    "user": "app",
    "password": "..."
}
```

**Учётные данные открытым текстом.** Разрешено, но при загрузке конфигурации пишет предупреждение в журнал. Это единственный случай, когда клиент согласится отправить пароль по нешифрованному каналу: при `starttls`, если TLS не поднялся, он откажется.

```json
"relay": {
    "host": "127.0.0.1",
    "security": "none",
    "user": "app",
    "password": "..."
}
```

**Релей вместе с собственным DKIM.** Сочетается, но обычно не нужно: релей подписывает своим ключом и своим доменом.

```json
"mail": {
    "dkim_private": "/etc/cwfr/dkim_private.pem",
    "dkim_selector": "mail",
    "host": "example.com",
    "relay": {
        "host": "smtp.mail.ru",
        "user": "info@example.com",
        "password": "...",
        "timeout": 15
    }
}
```

Список конфигураций, которые останавливают запуск, — в [config.md](/config#mail-relay).

## Настройка DKIM

### Генерация ключей

```bash
# Генерация приватного ключа
openssl genrsa -out dkim_private.pem 2048

# Извлечение публичного ключа
openssl rsa -in dkim_private.pem -pubout -out dkim_public.pem
```

### DNS-запись

Добавьте TXT-запись в DNS вашего домена:

```
mail._domainkey.example.com. IN TXT "v=DKIM1; k=rsa; p=<публичный_ключ>"
```

Где `mail` — это значение `dkim_selector` из конфигурации, а `example.com` — значение `host`.

::: tip Получение готовой строки `p=`
```bash
# Удалить заголовки PEM и перевести в одну строку
grep -v -- '----' dkim_public.pem | tr -d '\n'
```
:::

## API отправки почты

### Структура payload

```c
typedef struct mail_payload {
    const char* from;       // Email отправителя
    const char* from_name;  // Имя отправителя (кодируется в UTF-8)
    const char* to;         // Email получателя
    const char* subject;    // Тема письма (кодируется в UTF-8)
    const char* body;       // Тело письма (HTML, кодируется в base64)
} mail_payload_t;
```

### Синхронная отправка

```c
#include "mail.h"

int send_mail(mail_payload_t* payload);
```

Отправляет письмо синхронно, блокируя выполнение до завершения.

В режиме прямой доставки перед отправкой проверяется существование MX-записей домена получателя (`mail_is_real`), затем клиент подключается к MX-серверу. В режиме релея эта проверка **не выполняется**: маршрутизацией занимается релей, а у внутреннего домена MX может не быть вовсе.

**Параметры**\
`payload` — указатель на структуру с данными письма.

**Возвращаемое значение**\
`1` при успехе, `0` при ошибке (неверный адрес, нет MX, сбой соединения/TLS/AUTH/SMTP). Причину неудачи сообщает [`send_mail_result()`](#причина-неудачи).

<br>

### Асинхронная отправка

```c
void send_mail_async(mail_payload_t* payload);
```

Отправляет письмо асинхронно через менеджер задач. Содержимое `payload` **полностью копируется** (каждое поле дублируется через `strdup`), поэтому можно безопасно передавать стековые или короткоживущие структуры — они могут быть освобождены сразу после вызова.

**Параметры**\
`payload` — указатель на структуру с данными письма.

**Возвращаемое значение**\
Нет. Выполнение продолжается немедленно; ошибки логируются.

<br>

### Проверка email-адреса

```c
int mail_is_real(const char* email);
```

Проверяет, что у домена получателя есть MX-записи: извлекает домен, преобразует его в punycode (поддержка IDN) и резолвит MX. Это подтверждает, что домен способен принимать почту, но **не гарантирует**, что конкретный ящик существует.

**Параметры**\
`email` — email-адрес для проверки.

**Возвращаемое значение**\
Ненулевое значение, если у домена есть MX-записи; `0` при ошибке или отсутствии MX.

<br>

### Причина неудачи

```c
typedef struct mail_result {
    int status;                              // код ответа SMTP, 0 — ответа не было
    char error[SMTPRESPONSE_MESSAGE_SIZE];   // текст причины
} mail_result_t;

int send_mail_result(mail_payload_t* payload, mail_result_t* result);
```

`send_mail()` возвращает `0` на любую неудачу, а «ящика не существует» (5xx, повторять бессмысленно) и «попробуйте позже» (4xx, стоит повторить) — разные ответы. `send_mail_result()` делает ровно то же самое, что `send_mail()`, но заполняет переданную структуру причиной. Сам `send_mail()` — это и есть вызов с `result == NULL`, так что переходить на новую функцию нужно только там, где причина важна.

`status` — трёхзначный код ответа SMTP либо `0`, если сессия оборвалась раньше, чем пришёл хоть какой-то ответ (DNS, `connect`, TLS, отказ отправлять учётные данные открытым текстом). `error` — слова самого сервера, если ответ был, иначе название шага, на котором всё остановилось; пустая строка, если записывать нечего. Завершающего CRLF в тексте нет.

Структурой владеет вызывающий, поэтому ответ живёт ровно столько, сколько нужно: скрытого состояния нет, оговорки «действительно до следующего вызова» — тоже.

```c
mail_result_t result;

if (!send_mail_result(&payload, &result)) {
    if (result.status >= 400 && result.status < 500) {
        // временная ошибка — письмо имеет смысл поставить в очередь заново
        log_error("Mail deferred: %s\n", result.error);
    }
    else {
        // постоянная ошибка либо ответа не было вовсе
        log_error("Mail failed (%d): %s\n", result.status, result.error);
    }
}
```

::: warning `send_mail_async()` причину не сообщает
Асинхронная отправка выполняется на потоке менеджера задач, и к моменту, когда она закончится, обработчик запроса давно ответил клиенту. Спросить о причине неоткуда — она остаётся только в журнале. Если решение нужно принимать программно, отправляйте синхронно через `send_mail_result()`.
:::

::: tip
Сами повторы фреймворк не выполняет: `send_mail_async()` отдаёт задачу менеджеру задач и не хранит состояния. Решение о повторной постановке — за приложением.
:::

## Примеры использования

### Простая отправка

```c
#include "http.h"
#include "mail.h"

void send_welcome_email(httpctx_t* ctx) {
    mail_payload_t payload = {
        .from = "noreply@example.com",
        .from_name = "Example App",
        .to = "user@gmail.com",
        .subject = "Welcome!",
        .body = "Thank you for registering on our platform."
    };

    if (!send_mail(&payload)) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "Failed to send email");
        return;
    }

    ctx->response->send_data(ctx->response, "Email sent successfully");
}
```

### Асинхронная отправка

```c
void register_user(httpctx_t* ctx) {
    // ... создание пользователя ...

    // Отправляем письмо асинхронно (не блокирует ответ)
    mail_payload_t payload = {
        .from = "noreply@example.com",
        .from_name = "Example App",
        .to = user_email,
        .subject = "Confirm your email",
        .body = "Please click the link to confirm your email address."
    };

    // payload копируется — можно использовать стековую переменную
    send_mail_async(&payload);

    // Ответ отправляется немедленно
    ctx->response->send_data(ctx->response, "Registration successful");
}
```

### Проверка email перед регистрацией

```c
void validate_email(httpctx_t* ctx) {
    int ok = 0;
    const char* email = query_param_char(ctx->request->query_, "email", &ok);
    if (!ok || email == NULL) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Email is required");
        return;
    }

    if (!mail_is_real(email)) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Email domain cannot receive mail");
        return;
    }

    ctx->response->send_data(ctx->response, "Email is valid");
}
```

### HTML-письмо

```c
void send_html_email(httpctx_t* ctx) {
    const char* html_body =
        "<html>"
        "<head><style>body { font-family: Arial; }</style></head>"
        "<body>"
        "<h1>Welcome!</h1>"
        "<p>Thank you for joining us.</p>"
        "<a href=\"https://example.com/confirm\">Confirm Email</a>"
        "</body>"
        "</html>";

    mail_payload_t payload = {
        .from = "noreply@example.com",
        .from_name = "Example App",
        .to = "user@gmail.com",
        .subject = "Welcome to Example App",
        .body = html_body
    };

    send_mail(&payload);
    ctx->response->send_data(ctx->response, "HTML email sent");
}
```

## Расширенное использование

### Создание mail-объекта вручную

Для более тонкого контроля можно использовать низкоуровневый API `mail_t`. Поле `host` используется в `EHLO` и в домене `Message-Id`; `dkim_private` и `dkim_selector` нужны только для подписи и могут отсутствовать.

```c
#include "mail.h"

void send_custom_mail(void) {
    mail_t* mail = mail_create();
    if (mail == NULL) return;

    // Подключение. В прямом режиме — к MX получателя на порт 25; при заданном
    // mail.relay — к релею, и тогда аргумент-адрес не используется. Implicit
    // TLS (security: "tls") выполняет рукопожатие здесь же, до баннера.
    if (!mail->connect(mail, "recipient@example.com")) {
        mail->free(mail);
        return;
    }

    // Чтение баннера сервера (ожидается 220/250)
    if (!mail->read_banner(mail)) {
        mail->free(mail);
        return;
    }

    // EHLO (используется env()->mail.host); разбирает список расширений
    if (!mail->send_hello(mail)) {
        mail->free(mail);
        return;
    }

    // STARTTLS на том же соединении + повторный EHLO поверх TLS.
    // Не вызывайте при security: "tls" или "none".
    if (!mail->start_tls(mail)) {
        mail->free(mail);
        return;
    }

    // AUTH. Ничего не делает и возвращает 1 в прямом режиме и когда
    // учётные данные не заданы, поэтому вызывать можно безусловно.
    if (!mail->auth(mail)) {
        mail->free(mail);
        return;
    }

    // Установка отправителя/получателя/темы/тела
    mail->set_from(mail, "sender@example.com", "Sender Name");
    mail->set_to(mail, "recipient@example.com");
    mail->set_subject(mail, "Test Subject");
    mail->set_body(mail, "Test body content");

    // Отправка: MAIL FROM, RCPT TO, DATA, контент + DKIM
    if (!mail->send_mail(mail)) {
        // Ошибка отправки
    }

    // Сброс сессии и закрытие соединения
    mail->send_reset(mail);
    mail->send_quit(mail);
    mail->free(mail);
}
```

::: warning
Метод `send_mail` объекта `mail_t` выполняет только `MAIL FROM` → `RCPT TO` → `DATA` → передачу контента. Чтобы корректно завершить SMTP-сессию, дополнительно вызовите `send_reset` и `send_quit`, как показано выше. В высокоуровневую `send_mail()` это уже встроено.
:::

## Отладка

Для отладки проблем с отправкой почты:

**Прямая доставка:**

1. Проверьте правильность DNS-записей (MX, SPF, DKIM).
2. Убедитесь, что приватный ключ DKIM доступен для чтения процессом сервера.
3. Проверьте, что домен из `host` имеет обратную DNS-запись (FCrDNS), указывающую на ваш IP.
4. Проверьте логи приложения на наличие ошибок SMTP (`log_error` из модуля `mail`).

```bash
# Проверка MX-записей
dig MX example.com

# Проверка DKIM-записи
dig TXT mail._domainkey.example.com

# Проверка SPF-записи
dig TXT example.com
```

**Релей:**

| Симптом в журнале | Причина |
|-------------------|---------|
| `535 authentication failed` | Неверные `user`/`password`. Для Gmail нужен пароль приложения |
| `Relay does not offer AUTH` | Сервер не объявил `AUTH` в ответе на `EHLO` — обычно потому, что до него не дошло TLS |
| `Relay offers no supported AUTH mechanism` | Сервер предлагает только механизмы вне `PLAIN`/`LOGIN` |
| `Server does not offer STARTTLS` | Порт не submission-овый, либо релею нужен `security: "tls"` (465) |
| `Certificate verification failed` | Самоподписанный или несовпадающий сертификат — проверьте `relay.host` или выставьте `"verify": false` |
| `Refusing to send credentials over an unencrypted connection` | TLS не поднялся, а `security` не `none` |
| `Failed to connect` | Порт закрыт или недоступен; см. `timeout` |

Посмотреть сам диалог с релеем удобно локальным приёмником (Mailpit, MailHog): он поддерживает STARTTLS, implicit TLS и AUTH и показывает принятое письмо.

```bash
# Проверить, что релей вообще отвечает и что он объявляет
openssl s_client -starttls smtp -connect smtp.mail.ru:587 -crlf
```

::: warning Проверка на живом релее
Локальный приёмник не воспроизводит того, что есть у настоящего: лимиты отправки, требование совпадения `MAIL FROM` с логином и собственную DKIM-подпись релея. Перед вводом в эксплуатацию проверьте на реальном сервере.
:::
