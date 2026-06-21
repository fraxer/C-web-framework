---
outline: deep
description: Sending email in the C Web Framework. Built-in SMTP client with direct delivery to the recipient's MX server and DKIM signatures.
---

# Sending Email

The framework includes a built-in SMTP client for sending email with DKIM signatures. Delivery is performed directly to the recipient's MX server — no external SMTP relay and no credentials (login/password) required.

## How it works

1. The recipient's domain is extracted from the email address and converted to punycode (IDN support).
2. MX records are resolved for the domain; the client connects to the highest-priority server.
3. The initial connection is on port **25**, then `STARTTLS` is issued; the client switches to port **587** and re-issues `EHLO` over TLS.
4. The message is DKIM-signed (`From`, `To`, `Subject`, `Date`, `Message-Id` headers) and sent (`MAIL FROM`, `RCPT TO`, `DATA`).

::: tip
Because delivery is direct from your server, proper DNS records (MX, SPF, DKIM) and a clean, non-blacklisted IP are required for good deliverability.
:::

Content encoding:
- **Subject** and **sender name** are encoded as RFC 2047: `=?UTF-8?B?…?=` (any Unicode supported).
- **Body** is base64-encoded with line wrapping at 76 characters.
- Headers are always: `Content-Type: text/html; charset=utf-8` and `Content-Transfer-Encoding: base64` — so HTML emails work out of the box.

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
- `dkim_private` — path to the RSA DKIM private key (PEM). Required for signing.
- `dkim_selector` — DKIM selector; together with `host` it forms the `<selector>._domainkey.<host>` record.
- `host` — your sending domain. Used in the `EHLO` command, in the DKIM `d=` tag, and in the `Message-Id` domain. Must match the domain in your DKIM/SPF records.

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

Where `mail` is the `dkim_selector` value from configuration and `example.com` is the `host` value.

::: tip Producing the `p=` string
```bash
# Strip PEM headers and collapse to a single line
grep -v -- '----' dkim_public.pem | tr -d '\n'
```
:::

## Mail Sending API

### Payload Structure

```c
typedef struct mail_payload {
    const char* from;       // Sender email
    const char* from_name;  // Sender name (UTF-8 encoded)
    const char* to;         // Recipient email
    const char* subject;    // Email subject (UTF-8 encoded)
    const char* body;       // Email body (HTML, base64 encoded)
} mail_payload_t;
```

### Synchronous Sending

```c
#include "mail.h"

int send_mail(mail_payload_t* payload);
```

Sends an email synchronously, blocking execution until complete. Before sending, it verifies the recipient domain's MX records (`mail_is_real`), then connects to the MX server, DKIM-signs the message, and transmits it.

**Parameters**\
`payload` — pointer to a structure with the email data.

**Return Value**\
`1` on success, `0` on error (invalid address, no MX, connection/TLS/SMTP failure).

<br>

### Asynchronous Sending

```c
void send_mail_async(mail_payload_t* payload);
```

Sends an email asynchronously via the task manager. The `payload` is **fully copied** (each field is duplicated with `strdup`), so it is safe to pass stack-allocated or short-lived structures — they may be freed immediately after the call.

**Parameters**\
`payload` — pointer to a structure with the email data.

**Return Value**\
None. Execution continues immediately; errors are logged.

<br>

### Checking an Email Address

```c
int mail_is_real(const char* email);
```

Checks that the recipient domain has MX records: extracts the domain, converts it to punycode (IDN support), and resolves its MX records. This confirms the domain can receive mail but **does not** verify that the specific mailbox exists.

**Parameters**\
`email` — email address to check.

**Return Value**\
Non-zero if the domain has MX records; `0` on error or when no MX records exist.

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

    // Send email asynchronously (doesn't block the response)
    mail_payload_t payload = {
        .from = "noreply@example.com",
        .from_name = "Example App",
        .to = user_email,
        .subject = "Confirm your email",
        .body = "Please click the link to confirm your email address."
    };

    // payload is copied — a stack variable is fine
    send_mail_async(&payload);

    // Response is sent immediately
    ctx->response->send_data(ctx->response, "Registration successful");
}
```

### Email Validation Before Registration

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

### Creating a Mail Object Manually

For finer control, you can use the low-level `mail_t` API. The `dkim_private`, `dkim_selector`, and `host` config values are still required — they are used to build the DKIM signature and the `Message-Id` during content transmission.

```c
#include "mail.h"

void send_custom_mail(void) {
    mail_t* mail = mail_create();
    if (mail == NULL) return;

    // Connect to the recipient's MX server (port 25)
    if (!mail->connect(mail, "recipient@example.com")) {
        mail->free(mail);
        return;
    }

    // Read the server banner (expects 220/250)
    if (!mail->read_banner(mail)) {
        mail->free(mail);
        return;
    }

    // EHLO (uses env()->mail.host)
    if (!mail->send_hello(mail)) {
        mail->free(mail);
        return;
    }

    // STARTTLS + a second EHLO (port switches to 587)
    if (!mail->start_tls(mail)) {
        mail->free(mail);
        return;
    }

    // Set sender/recipient/subject/body
    mail->set_from(mail, "sender@example.com", "Sender Name");
    mail->set_to(mail, "recipient@example.com");
    mail->set_subject(mail, "Test Subject");
    mail->set_body(mail, "Test body content");

    // Send: MAIL FROM, RCPT TO, DATA, content + DKIM
    if (!mail->send_mail(mail)) {
        // Sending error
    }

    // Reset the session and close the connection
    mail->send_reset(mail);
    mail->send_quit(mail);
    mail->free(mail);
}
```

::: warning
The `mail_t` object's `send_mail` method only performs `MAIL FROM` → `RCPT TO` → `DATA` → content transmission. To cleanly end the SMTP session, also call `send_reset` and `send_quit` as shown above. The high-level `send_mail()` function already does this for you.
:::

## Debugging

To debug mail sending issues:

1. Check that DNS records are correct (MX, SPF, DKIM).
2. Ensure the DKIM private key is readable by the server process.
3. Verify the `host` domain has forward-confirmed reverse DNS (FCrDNS) pointing back to your IP.
4. Check application logs for SMTP errors (`log_error` from the `mail` module).

```bash
# Check MX records
dig MX example.com

# Check DKIM record
dig TXT mail._domainkey.example.com

# Check SPF record
dig TXT example.com
```
