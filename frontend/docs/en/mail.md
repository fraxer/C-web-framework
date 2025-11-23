---
outline: deep
description: Sending email in C Web Framework. SMTP client with DKIM signature support for sender authentication.
---

# Sending Email

The framework includes a built-in SMTP client for sending emails with DKIM signature support.

## Configuration

Mail settings are specified in the `config.json` file:

```json
{
    "mail": {
        "dkim_private": "/path/to/dkim_private.pem",
        "dkim_selector": "mail",
        "host": "example.com"
    }
}
```

**Parameters:**
- `dkim_private` — path to DKIM private key
- `dkim_selector` — DKIM selector (used in DNS record)
- `host` — sender domain

## DKIM Setup

### Key Generation

```bash
# Generate private key
openssl genrsa -out dkim_private.pem 2048

# Extract public key
openssl rsa -in dkim_private.pem -pubout -out dkim_public.pem
```

### DNS Record

Add a TXT record to your domain's DNS:

```
mail._domainkey.example.com. IN TXT "v=DKIM1; k=rsa; p=<public_key>"
```

Where `mail` is the `dkim_selector` value from configuration.

## Mail Sending API

### Payload Structure

```c
typedef struct mail_payload {
    const char* from;       // Sender email
    const char* from_name;  // Sender name
    const char* to;         // Recipient email
    const char* subject;    // Email subject
    const char* body;       // Email body
} mail_payload_t;
```

### Synchronous Sending

```c
#include "mail.h"

int send_mail(mail_payload_t* payload);
```

Sends an email synchronously, blocking execution until complete.

**Parameters**\
`payload` — pointer to structure with email data.

**Return Value**\
Non-zero value on success, zero on error.

<br>

### Asynchronous Sending

```c
void send_mail_async(mail_payload_t* payload);
```

Sends an email asynchronously in the background.

**Parameters**\
`payload` — pointer to structure with email data.

**Return Value**\
None. Execution continues immediately.

<br>

### Checking Email Existence

```c
int mail_is_real(const char* email);
```

Checks if an email address exists via MX records and SMTP dialog.

**Parameters**\
`email` — email address to check.

**Return Value**\
Non-zero value if the address exists.

## Usage Examples

### Simple Sending

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

### Asynchronous Sending

```c
void register_user(httpctx_t* ctx) {
    // ... create user ...

    // Send email asynchronously (doesn't block response)
    mail_payload_t payload = {
        .from = "noreply@example.com",
        .from_name = "Example App",
        .to = user_email,
        .subject = "Confirm your email",
        .body = "Please click the link to confirm your email address."
    };

    send_mail_async(&payload);

    // Response is sent immediately
    ctx->response->send_data(ctx->response, "Registration successful");
}
```

### Email Validation Before Registration

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

### HTML Email

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

## Advanced Usage

### Creating Mail Object Manually

For more fine-grained control, you can use the low-level API:

```c
#include "mail.h"

void send_custom_mail(void) {
    mail_t* mail = mail_create();
    if (mail == NULL) return;

    // Connect to recipient's server
    if (!mail->connect(mail, "recipient@example.com")) {
        mail->free(mail);
        return;
    }

    // Read server banner
    if (!mail->read_banner(mail)) {
        mail->free(mail);
        return;
    }

    // EHLO
    if (!mail->send_hello(mail)) {
        mail->free(mail);
        return;
    }

    // Start TLS (if supported)
    mail->start_tls(mail);

    // Set sender
    mail->set_from(mail, "sender@example.com", "Sender Name");

    // Set recipient
    mail->set_to(mail, "recipient@example.com");

    // Subject and body
    mail->set_subject(mail, "Test Subject");
    mail->set_body(mail, "Test body content");

    // Send
    if (!mail->send_mail(mail)) {
        // Sending error
    }

    // Finish
    mail->send_quit(mail);
    mail->free(mail);
}
```

## Debugging

To debug mail sending issues:

1. Check DNS records are correct (MX, SPF, DKIM)
2. Ensure DKIM private key is readable
3. Check application logs for SMTP errors

```bash
# Check MX records
dig MX example.com

# Check DKIM record
dig TXT mail._domainkey.example.com

# Check SPF record
dig TXT example.com
```
