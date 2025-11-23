---
outline: deep
description: Отправка email в C Web Framework. SMTP-клиент с поддержкой DKIM-подписей для аутентификации отправителя.
---

# Отправка Email

Фреймворк включает встроенный SMTP-клиент для отправки электронной почты с поддержкой DKIM-подписей.

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
- `dkim_private` — путь к приватному ключу DKIM
- `dkim_selector` — селектор DKIM (используется в DNS-записи)
- `host` — домен отправителя

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

Где `mail` — это значение `dkim_selector` из конфигурации.

## API отправки почты

### Структура payload

```c
typedef struct mail_payload {
    const char* from;       // Email отправителя
    const char* from_name;  // Имя отправителя
    const char* to;         // Email получателя
    const char* subject;    // Тема письма
    const char* body;       // Тело письма
} mail_payload_t;
```

### Синхронная отправка

```c
#include "mail.h"

int send_mail(mail_payload_t* payload);
```

Отправляет письмо синхронно, блокируя выполнение до завершения.

**Параметры**\
`payload` — указатель на структуру с данными письма.

**Возвращаемое значение**\
Ненулевое значение при успехе, ноль при ошибке.

<br>

### Асинхронная отправка

```c
void send_mail_async(mail_payload_t* payload);
```

Отправляет письмо асинхронно в фоновом режиме.

**Параметры**\
`payload` — указатель на структуру с данными письма.

**Возвращаемое значение**\
Нет. Выполнение продолжается немедленно.

<br>

### Проверка существования email

```c
int mail_is_real(const char* email);
```

Проверяет существование email-адреса через MX-записи и SMTP-диалог.

**Параметры**\
`email` — email-адрес для проверки.

**Возвращаемое значение**\
Ненулевое значение, если адрес существует.

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

    send_mail_async(&payload);

    // Ответ отправляется немедленно
    ctx->response->send_data(ctx->response, "Registration successful");
}
```

### Проверка email перед регистрацией

```c
void validate_email(httpctx_t* ctx) {
    int ok = 0;
    const char* email = query_param_char(ctx->request, "email", &ok);
    if (!ok || email == NULL) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Email is required");
        return;
    }

    if (!mail_is_real(email)) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Email does not exist");
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

Для более тонкого контроля можно использовать низкоуровневый API:

```c
#include "mail.h"

void send_custom_mail(void) {
    mail_t* mail = mail_create();
    if (mail == NULL) return;

    // Подключение к серверу получателя
    if (!mail->connect(mail, "recipient@example.com")) {
        mail->free(mail);
        return;
    }

    // Чтение баннера сервера
    if (!mail->read_banner(mail)) {
        mail->free(mail);
        return;
    }

    // EHLO
    if (!mail->send_hello(mail)) {
        mail->free(mail);
        return;
    }

    // Начало TLS (если поддерживается)
    mail->start_tls(mail);

    // Установка отправителя
    mail->set_from(mail, "sender@example.com", "Sender Name");

    // Установка получателя
    mail->set_to(mail, "recipient@example.com");

    // Тема и тело
    mail->set_subject(mail, "Test Subject");
    mail->set_body(mail, "Test body content");

    // Отправка
    if (!mail->send_mail(mail)) {
        // Ошибка отправки
    }

    // Завершение
    mail->send_quit(mail);
    mail->free(mail);
}
```

## Отладка

Для отладки проблем с отправкой почты:

1. Проверьте правильность DNS-записей (MX, SPF, DKIM)
2. Убедитесь, что приватный ключ DKIM доступен для чтения
3. Проверьте логи приложения на наличие ошибок SMTP

```bash
# Проверка MX-записей
dig MX example.com

# Проверка DKIM-записи
dig TXT mail._domainkey.example.com

# Проверка SPF-записи
dig TXT example.com
```
