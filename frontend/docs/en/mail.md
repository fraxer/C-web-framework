---
outline: deep
description: "Sending email in the C Web Framework. Built-in SMTP client: direct delivery to the recipient's MX server or submission through an authenticated SMTP relay, with DKIM signatures."
---

# Sending Email

The framework includes a built-in SMTP client with two delivery modes:

- **direct delivery** — the message goes straight to the recipient's MX server, with no external service and no credentials. This is the default;
- **relay delivery** — the message is handed to a configured SMTP server (`smtp.mail.ru`, `smtp.yandex.ru`, a corporate relay) with a login and password.

The mode is selected by **configuration**, not by code: adding a `mail.relay` object to `config.json` switches delivery to the relay. `send_mail()`, `send_mail_async()` and `mail_payload_t` are identical in both modes — the application never learns which path the message took.

## Choosing a mode

Direct delivery needs no external service, but it does need reputation: without correct PTR, SPF and DKIM records, and from an IP that has not been warmed up, the message lands in spam or is rejected outright (`550 spam message rejected`). It is the right choice when the domain and address already have a good standing.

A relay is what you want when that reputation does not exist and building one for the sake of a contact form is not worth it: the relay provider carries the reputation, and usually signs the message with its own DKIM key.

## How it works

### Direct delivery

1. The recipient's domain is extracted from the email address and converted to punycode (IDN support).
2. MX records are resolved for the domain; the client connects to the highest-priority server on port **25**.
3. `EHLO` is issued, then `STARTTLS` — TLS is negotiated **on the same connection**, the port does not change, and `EHLO` is repeated over TLS.
4. The message is DKIM-signed (when a key is configured) and sent (`MAIL FROM`, `RCPT TO`, `DATA`).

::: tip
Because delivery is direct from your server, proper DNS records (MX, SPF, DKIM) and a clean, non-blacklisted IP are required for good deliverability.
:::

### Through a relay

1. The relay's name is resolved with `getaddrinfo()` (A and AAAA — an IPv6 relay is supported); every returned address is tried until one connects on the configured port. No MX query is made at all, and the recipient's domain is not required to have MX records.
2. With `security: "tls"` the TLS handshake runs immediately after `connect`, **before** the banner is read; no `STARTTLS` is sent.
3. `EHLO` is issued, and the extension list (`STARTTLS`, `SIZE`, `AUTH`) is parsed out of the multi-line reply.
4. With `security: "starttls"`, `STARTTLS` is sent — but only if the server announced it — and `EHLO` is repeated over TLS: servers normally announce their `AUTH` mechanisms only once the session is encrypted.
5. If `user` and `password` are configured, `AUTH` is performed (see [Authentication](#authentication)).
6. From there it is the same as direct delivery: `MAIL FROM`, `RCPT TO`, `DATA`.

Content encoding (in both modes):
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
- `dkim_private` — path to the RSA DKIM private key (PEM). Optional: without it messages go out unsigned.
- `dkim_selector` — DKIM selector; together with `host` it forms the `<selector>._domainkey.<host>` record.
- `host` — your sending domain. Used in the `EHLO` command, in the DKIM `d=` tag, and in the `Message-Id` domain. Must match the domain in your DKIM/SPF records.

::: warning DKIM is all or nothing
`dkim_private` and `dkim_selector` are configured **together**. One without the other is a configuration error that stops startup: it cannot produce a signature, and quietly sending unsigned would be the wrong reading of a typo. With neither of them set, DKIM is simply not applied.
:::

### The relay section

The presence of the `relay` object is the mode switch.

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

| Field | Required | Default | Description |
|-------|----------|---------|-------------|
| `host` | **yes** | — | Name or address of the relay server |
| `port` | no | per `security`: 587 / 465 / 25 | Port |
| `security` | no | `starttls` | `starttls`, `tls` (implicit TLS), `none` |
| `user` | no | — | Login; without it no `AUTH` is performed |
| `password` | no | — | Password |
| `auth` | no | `auto` | `auto`, `plain`, `login`, `none` |
| `timeout` | no | 30 | Socket operation timeout, seconds |
| `verify` | no | `true` | Verify the relay's certificate and hostname |

Checks performed at configuration load:

- `user` without `password` (or the reverse) is a configuration error, not a silent send without `AUTH`;
- `security: "none"` with no `user` is valid: an internal relay that accepts mail from the local network;
- `security: "none"` together with `user` produces a warning in the log — the credentials will travel in the clear. It is allowed, but the client will only send a password over an unencrypted channel when `security: "none"` was stated explicitly.

The password never reaches any log, and the buffers holding it are wiped with `explicit_bzero()` immediately after use.

::: tip Keeping the password out of the repository
`config.json` is conveniently assembled from a template in the container entrypoint, with values substituted from `.env` — that keeps the password out of version control. The framework introduces no separate secrets mechanism.
:::

### Authentication

Two mechanisms are supported — the ones every common relay offers:

- **`PLAIN`** (RFC 4616) — a single command whose argument is `base64("\0" + login + "\0" + password)`;
- **`LOGIN`** — three steps: `AUTH LOGIN`, then the login and the password one at a time, each base64-encoded, each in answer to a `334` challenge.

With `auth: "auto"`, `PLAIN` is chosen when the server announces it, otherwise `LOGIN`. If a specific mechanism is configured and the server does not offer it, the send stops with a message naming what the server actually offered; there is no silent fallback to another mechanism.

`AUTH` runs **after** TLS is established. A `535` reply (bad credentials) gets a log message of its own — it is the most common configuration mistake.

::: tip Gmail
`smtp.gmail.com` requires an app password rather than the account password. `CRAM-MD5` and `XOAUTH2` are not supported.
:::

### TLS and certificate verification

With `verify: true` (the default) the system trust store is loaded for the relay, `SSL_VERIFY_PEER` is enabled and the hostname is checked; `relay.host` is sent in SNI (unless it is an IP address). A self-signed certificate on an internal relay is a legitimate reason to set `"verify": false`.

For **direct** delivery the MX server's certificate is not verified, and that is deliberate: delivery to a stranger's MX is opportunistic TLS (RFC 7435), where certificates routinely fail to match the MX name, and refusing such connections would mean not delivering at all.

### Ready-made variants

**Direct delivery, signed.** The default mode: there is no `relay` section.

```json
"mail": {
    "dkim_private": "/etc/cwfr/dkim_private.pem",
    "dkim_selector": "mail",
    "host": "example.com"
}
```

**Direct delivery, unsigned.** The `mail` section can be dropped entirely, or reduced to `host` — messages go to the recipient's MX with no `DKIM-Signature`.

```json
"mail": {
    "host": "example.com"
}
```

**Relay on submission (587).** The common case; everything but the mandatory `host` is defaulted: `port: 587`, `security: "starttls"`, `auth: "auto"`, `timeout: 30`, `verify: true`. No DKIM is configured — the relay signs the message itself, which is how it usually goes.

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

**Implicit TLS (465).** The port is filled in automatically; the handshake runs before the banner and no `STARTTLS` is sent.

```json
"relay": {
    "host": "smtp.yandex.ru",
    "security": "tls",
    "user": "info@example.com",
    "password": "..."
}
```

**An explicitly chosen mechanism.** If the server offers only the other one, the send stops with a message naming what it actually offered; there is no silent fallback. Gmail requires an app password.

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

**Internal relay with no authentication.** The port becomes 25 and no `AUTH` is performed. A valid configuration; nothing is warned about.

```json
"relay": {
    "host": "postfix.internal",
    "security": "none"
}
```

**Internal relay with a self-signed certificate.** Without `"verify": false` such a connection is refused.

```json
"relay": {
    "host": "smtp.internal.lan",
    "security": "tls",
    "verify": false,
    "user": "app",
    "password": "..."
}
```

**Credentials in the clear.** Allowed, but the configuration loader logs a warning. This is the only case in which the client will send a password over an unencrypted channel: with `starttls`, if TLS did not come up, it refuses.

```json
"relay": {
    "host": "127.0.0.1",
    "security": "none",
    "user": "app",
    "password": "..."
}
```

**A relay together with your own DKIM.** The two combine, but this is rarely what you want: the relay signs with its own key and its own domain.

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

The configurations that stop start-up are listed in [config.md](/en/config#mail-relay).

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

Sends an email synchronously, blocking execution until complete.

In direct-delivery mode the recipient domain's MX records are verified first (`mail_is_real`), and the client then connects to the MX server. In relay mode that check is **not** performed: routing is the relay's job, and an internal domain may have no MX records at all.

**Parameters**\
`payload` — pointer to a structure with the email data.

**Return Value**\
`1` on success, `0` on error (invalid address, no MX, connection/TLS/AUTH/SMTP failure). The reason is reported by [`send_mail_result()`](#why-a-send-failed).

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

<br>

### Why a send failed

```c
typedef struct mail_result {
    int status;                              // SMTP reply code, 0 if no reply arrived
    char error[SMTPRESPONSE_MESSAGE_SIZE];   // the reason, as text
} mail_result_t;

int send_mail_result(mail_payload_t* payload, mail_result_t* result);
```

`send_mail()` returns `0` for every kind of failure, and "the mailbox does not exist" (5xx, never worth retrying) is not the same answer as "try again later" (4xx, worth retrying). `send_mail_result()` does exactly what `send_mail()` does and fills in the struct you hand it. `send_mail()` *is* the call with `result == NULL`, so there is no reason to move to the new function except where the reason matters.

`status` is the three-digit SMTP reply code, or `0` when the session failed before any reply arrived (DNS, `connect`, TLS, or a refusal to send credentials in the clear). `error` holds the server's own words when there was a reply, and otherwise the name of the step that gave up; it is empty when there is nothing to report, and never carries a trailing CRLF.

The caller owns the struct, so the answer lives exactly as long as it is wanted: no hidden state, and no "valid until the next call" caveat.

```c
mail_result_t result;

if (!send_mail_result(&payload, &result)) {
    if (result.status >= 400 && result.status < 500) {
        // temporary — the message is worth re-queueing
        log_error("Mail deferred: %s\n", result.error);
    }
    else {
        // permanent, or no reply was ever received
        log_error("Mail failed (%d): %s\n", result.status, result.error);
    }
}
```

::: warning `send_mail_async()` reports no reason
An asynchronous send runs on a task-manager thread, and by the time it finishes the request handler has long since answered the client. There is nobody to report to — the reason stays in the log. When the decision has to be made in code, send synchronously with `send_mail_result()`.
:::

::: tip
The framework performs no retries of its own: `send_mail_async()` hands the task to the task manager and keeps no state. Re-queueing is the application's decision.
:::

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

For finer control, you can use the low-level `mail_t` API. `host` is used for `EHLO` and the `Message-Id` domain; `dkim_private` and `dkim_selector` are needed only for the signature and may be absent.

```c
#include "mail.h"

void send_custom_mail(void) {
    mail_t* mail = mail_create();
    if (mail == NULL) return;

    // Connect. In direct mode to the recipient's MX on port 25; with
    // mail.relay configured, to the relay — and then the address argument is
    // unused. Implicit TLS (security: "tls") handshakes here, before the banner.
    if (!mail->connect(mail, "recipient@example.com")) {
        mail->free(mail);
        return;
    }

    // Read the server banner (expects 220/250)
    if (!mail->read_banner(mail)) {
        mail->free(mail);
        return;
    }

    // EHLO (uses env()->mail.host); parses the extension list
    if (!mail->send_hello(mail)) {
        mail->free(mail);
        return;
    }

    // STARTTLS on the same connection + a second EHLO over TLS.
    // Do not call it with security: "tls" or "none".
    if (!mail->start_tls(mail)) {
        mail->free(mail);
        return;
    }

    // AUTH. A no-op returning 1 in direct mode and when no credentials are
    // configured, so it is safe to call unconditionally.
    if (!mail->auth(mail)) {
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

**Direct delivery:**

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

**Relay:**

| Log message | Cause |
|-------------|-------|
| `535 authentication failed` | Wrong `user`/`password`. Gmail needs an app password |
| `Relay does not offer AUTH` | The server announced no `AUTH` in its `EHLO` reply — usually because TLS was never established |
| `Relay offers no supported AUTH mechanism` | The server offers only mechanisms other than `PLAIN`/`LOGIN` |
| `Server does not offer STARTTLS` | Not a submission port, or the relay wants `security: "tls"` (465) |
| `Certificate verification failed` | Self-signed or mismatched certificate — check `relay.host`, or set `"verify": false` |
| `Refusing to send credentials over an unencrypted connection` | TLS did not come up and `security` is not `none` |
| `Failed to connect` | The port is closed or unreachable; see `timeout` |

The easiest way to watch the dialogue with a relay is a local receiver (Mailpit, MailHog): it supports STARTTLS, implicit TLS and AUTH, and shows the accepted message.

```bash
# Check that the relay answers, and what it announces
openssl s_client -starttls smtp -connect smtp.mail.ru:587 -crlf
```

::: warning Test against the real relay
A local receiver does not reproduce what a production relay has: sending limits, the requirement that `MAIL FROM` match the login, and the relay's own DKIM signature. Verify against the real server before going live.
:::
