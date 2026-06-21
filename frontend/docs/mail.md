---
outline: deep
description: Отправка email в C Web Framework. Встроенный SMTP-клиент с прямой доставкой на MX-сервер получателя и DKIM-подписями.
---

# Отправка Email

Фреймворк включает встроенный SMTP-клиент для отправки почты с DKIM-подписями. Доставка выполняется напрямую на MX-сервер получателя — без внешнего SMTP-рельея и без учётных данных (логина/пароля).

## Как это работает

1. Из email-адреса получателя извлекается домен и преобразуется в punycode (поддержка IDN).
2. По домену резолвятся MX-записи; клиент подключается к серверу с наивысшим приоритетом.
3. Исходное соединение устанавливается на порт **25**, затем выполняется команда `STARTTLS`, после чего клиент переключается на порт **587** и повторяет `EHLO` поверх TLS.
4. Письмо подписывается DKIM (заголовки `From`, `To`, `Subject`, `Date`, `Message-Id`) и отправляется (`MAIL FROM`, `RCPT TO`, `DATA`).

::: tip
Поскольку доставка идёт напрямую с вашего сервера, для нормальной доставляемости нужны корректные DNS-записи (MX, SPF, DKIM) и «чистый» IP, не внесённый в чёрные списки.
:::

Кодирование содержимого:
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
- `dkim_private` — путь к приватному RSA-ключу DKIM (PEM). Обязателен для подписи.
- `dkim_selector` — селектор DKIM; вместе с `host` образует запись `<selector>._domainkey.<host>`.
- `host` — ваш домен отправителя. Используется в команде `EHLO`, в теге DKIM `d=` и в домене `Message-Id`. Должен совпадать с доменом из DKIM/SPF.

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

Отправляет письмо синхронно, блокируя выполнение до завершения. Перед отправкой проверяет существование MX-записей домена получателя (`mail_is_real`), затем подключается к MX-серверу, подписывает письмо DKIM и передаёт его.

**Параметры**\
`payload` — указатель на структуру с данными письма.

**Возвращаемое значение**\
`1` при успехе, `0` при ошибке (неверный адрес, нет MX, сбой соединения/TLS/SMTP).

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

Для более тонкого контроля можно использовать низкоуровневый API `mail_t`. Поля конфигурации `dkim_private`, `dkim_selector` и `host` по-прежнему обязательны — они нужны для формирования DKIM-подписи и `Message-Id` на этапе отправки контента.

```c
#include "mail.h"

void send_custom_mail(void) {
    mail_t* mail = mail_create();
    if (mail == NULL) return;

    // Подключение к MX-серверу получателя (порт 25)
    if (!mail->connect(mail, "recipient@example.com")) {
        mail->free(mail);
        return;
    }

    // Чтение баннера сервера (ожидается 220/250)
    if (!mail->read_banner(mail)) {
        mail->free(mail);
        return;
    }

    // EHLO (используется env()->mail.host)
    if (!mail->send_hello(mail)) {
        mail->free(mail);
        return;
    }

    // STARTTLS + повторный EHLO (порт переключается на 587)
    if (!mail->start_tls(mail)) {
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
