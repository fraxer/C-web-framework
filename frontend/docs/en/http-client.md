---
outline: deep
description: HTTP client in C Web Framework. Sending GET, POST requests, working with headers, forms, JSON, TLS and redirects.
---

# HTTP Client

The framework includes a built-in synchronous HTTP client for making outgoing requests to external services. It supports HTTPS (TLS 1.2+), automatic redirect following, keep-alive connection pooling, and transparently handles requests to the server's own domains without a network round-trip (self-invocation).

## Initialization

```c
#include "httpclient.h"

httpclient_t* httpclient_init(route_methods_e method, const char* url, int timeout);
```

Creates a new HTTP client, parses the URL, and prepares the request and response objects.

**Parameters**\
`method` — HTTP method (`ROUTE_GET`, `ROUTE_POST`, `ROUTE_PUT`, `ROUTE_DELETE`, `ROUTE_PATCH`, `ROUTE_HEAD`, `ROUTE_OPTIONS`).\
`url` — absolute URL (`http://` or `https://`); internationalized domain names (IDN) are supported and converted to punycode automatically.\
`timeout` — socket read/write timeout in seconds. When `timeout <= 0`, the default of `10` seconds is used.

**Return Value**\
Pointer to the client or `NULL` on error (invalid URL, out of memory, OpenSSL initialization failure).

::: tip Default behavior
- An `https://` scheme enables TLS; peer certificate verification is **enabled** and the system trusted CAs are used.
- The `Host` and `Content-Length` headers are set automatically — no need to set them by hand.
:::

::: warning Timeout is set only at initialization
The timeout cannot be changed after the client is created: the `set_timeout` field is declared on the struct but not wired up. Pass the desired value as the `timeout` argument to `httpclient_init()`.
:::

## Client configuration

```c
void client->set_method(httpclient_t* client, route_methods_e method);
int  client->set_url(httpclient_t* client, const char* url);
void client->set_verify(httpclient_t* client, int verify, const char* ca_path);
```

| Method | Description |
|--------|-------------|
| `set_method` | Changes the HTTP method of the request. |
| `set_url` | Re-parses the URL and replaces the scheme, host, and port. Returns `1` on success, `0` on parse error. |
| `set_verify` | Configures TLS peer certificate verification. |

### TLS certificate verification

By default `verify_peer = 1`: for `https://` requests the peer certificate is verified against the trusted CAs. Use `set_verify` to override this:

```c
// Use a custom set of trusted certificates (CA bundle)
client->set_verify(client, 1, "/etc/ssl/certs/custom-ca-bundle.pem");

// Fall back to the system CA paths
client->set_verify(client, 1, NULL);

// Disable verification (insecure — tests/internal services only)
client->set_verify(client, 0, NULL);
```

## Sending a request

```c
httpresponse_t* client->send(httpclient_t* client);
```

Executes the HTTP request. The method **always returns a pointer to the response object** (`client->response`); it is never `NULL`. On a connection error, TLS failure, or exceeded redirect limit, the response's `status_code` is set to `500`.

```c
httpresponse_t* res = client->send(client);

if (res->status_code != 200) {
    // handle the external service error
}
```

### Redirect following

The client automatically follows `301` and `302` responses by reading the `Location` header. For non-safe methods (`POST`, `PUT`, `DELETE`, …) the method is switched to `GET` and the request body is dropped (RFC 7231). Up to 10 redirects are followed — exceeding the limit yields a response with `status_code = 500`.

## Reading the response

```c
// HTTP status of the response
int status = res->status_code;

// Response body as a string (malloc-allocated — free it with free()).
// Automatically decompressed when Content-Encoding: gzip is present.
char* body = res->get_payload(res);

// Response body as a JSON document (free it with json_free())
json_doc_t* doc = res->get_payload_json(res);

// Response header by name (no copy)
http_header_t* ct = res->get_header(res, "Content-Type");
```

## Freeing resources

```c
void client->free(httpclient_t* client);
```

Frees the client and all related objects (request, response, SSL context, buffers). Safe to call with `NULL`. Call it on every exit path of your handler.

## Usage Examples

### GET Request

```c
#include "http.h"
#include "httpclient.h"

void fetch_data(httpctx_t* ctx) {
    const char* url = "https://api.example.com/data";
    const int timeout = 5;

    httpclient_t* client = httpclient_init(ROUTE_GET, url, timeout);
    if (client == NULL) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(500));
        return;
    }

    httprequest_t* req = client->request;
    req->add_header(req, "Accept", "application/json");
    req->add_header(req, "User-Agent", "C-Web-Framework/1.0");

    httpresponse_t* res = client->send(client);
    if (res->status_code != 200) {
        ctx->response->status_code = res->status_code;
        ctx->response->send_data(ctx->response, "External API error");
        client->free(client);
        return;
    }

    char* body = res->get_payload(res);

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, body ? body : "");

    free(body);
    client->free(client);
}
```

### POST with URL-encoded Data

```c
void post_urlencoded(httpctx_t* ctx) {
    const char* url = "https://api.example.com/login";
    httpclient_t* client = httpclient_init(ROUTE_POST, url, 5);
    if (client == NULL) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(500));
        return;
    }

    httprequest_t* req = client->request;
    req->append_urlencoded(req, "username", "john");
    req->append_urlencoded(req, "password", "secret123");

    httpresponse_t* res = client->send(client);
    if (res->status_code != 200) {
        ctx->response->status_code = res->status_code;
        ctx->response->send_data(ctx->response, "Request failed");
        client->free(client);
        return;
    }

    char* body = res->get_payload(res);
    ctx->response->send_data(ctx->response, body ? body : "");

    free(body);
    client->free(client);
}
```

### POST with JSON Body

```c
void post_json(httpctx_t* ctx) {
    const char* url = "https://api.example.com/users";
    httpclient_t* client = httpclient_init(ROUTE_POST, url, 5);
    if (client == NULL) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(500));
        return;
    }

    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);
    json_object_set(root, "name", json_create_string("John Doe"));
    json_object_set(root, "email", json_create_string("john@example.com"));
    json_object_set(root, "age", json_create_number(30));

    httprequest_t* req = client->request;
    req->set_payload_json(req, doc);
    json_free(doc);

    httpresponse_t* res = client->send(client);
    ctx->response->status_code = res->status_code;
    ctx->response->add_header(ctx->response, "Content-Type", "application/json");

    char* body = res->get_payload(res);
    ctx->response->send_data(ctx->response, body ? body : "{}");

    free(body);
    client->free(client);
}
```

### Arbitrary Body (raw / text)

```c
void post_raw(httpctx_t* ctx) {
    httpclient_t* client = httpclient_init(ROUTE_POST, "https://api.example.com/echo", 5);
    if (client == NULL) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(500));
        return;
    }

    httprequest_t* req = client->request;

    // Raw bytes with an explicit Content-Type and length
    req->set_payload_raw(req, "application/octet-stream", 5, "hello");

    // Or just text (Content-Type: text/plain)
    // req->set_payload_text(req, "hello");

    httpresponse_t* res = client->send(client);
    char* body = res->get_payload(res);
    ctx->response->status_code = res->status_code;
    ctx->response->send_data(ctx->response, body ? body : "");

    free(body);
    client->free(client);
}
```

### POST with multipart/form-data

```c
void post_formdata(httpctx_t* ctx) {
    const char* url = "https://api.example.com/upload";
    httpclient_t* client = httpclient_init(ROUTE_POST, url, 10);
    if (client == NULL) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(500));
        return;
    }

    httprequest_t* req = client->request;

    // Text field
    req->append_formdata_text(req, "description", "My file description");

    // Raw data with a content type
    req->append_formdata_raw(req, "metadata", "application/json", "{\"type\":\"image\"}");

    // JSON object
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);
    json_object_set(root, "tags", json_create_string("photo,vacation"));
    req->append_formdata_json(req, "options", doc);
    json_free(doc);

    // File from a named storage (file_t)
    file_t file = storage_file_get("local", "uploads/image.png");
    if (file.ok) {
        req->append_formdata_file(req, "file", &file);
        file.close(&file);
    }

    // Or a file directly by filesystem path
    // req->append_formdata_filepath(req, "file", "/var/data/uploads/image.png");

    httpresponse_t* res = client->send(client);
    char* body = res->get_payload(res);
    ctx->response->status_code = res->status_code;
    ctx->response->send_data(ctx->response, body ? body : "Success");

    free(body);
    client->free(client);
}
```

### Request with Authorization

```c
void authorized_request(httpctx_t* ctx) {
    const char* url = "https://api.example.com/protected";
    httpclient_t* client = httpclient_init(ROUTE_GET, url, 5);
    if (client == NULL) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(500));
        return;
    }

    httprequest_t* req = client->request;

    // Bearer token
    req->add_header(req, "Authorization", "Bearer your-jwt-token-here");

    // Or Basic authorization
    // req->add_header(req, "Authorization", "Basic dXNlcjpwYXNz");

    httpresponse_t* res = client->send(client);
    if (res->status_code == 401) {
        ctx->response->status_code = 401;
        ctx->response->send_data(ctx->response, "Unauthorized");
        client->free(client);
        return;
    }

    char* body = res->get_payload(res);
    ctx->response->send_data(ctx->response, body ? body : "");

    free(body);
    client->free(client);
}
```

### HTTPS with Custom Certificate Verification

```c
void fetch_secure(httpctx_t* ctx) {
    httpclient_t* client = httpclient_init(ROUTE_GET, "https://internal.example.com/health", 5);
    if (client == NULL) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(500));
        return;
    }

    // Trust only the corporate CA bundle
    client->set_verify(client, 1, "/etc/ssl/certs/corporate-ca.pem");

    httpresponse_t* res = client->send(client);
    ctx->response->status_code = res->status_code;

    char* body = res->get_payload(res);
    ctx->response->send_data(ctx->response, body ? body : "");

    free(body);
    client->free(client);
}
```

### Handling a gzip Response

```c
void fetch_compressed(httpctx_t* ctx) {
    httpclient_t* client = httpclient_init(ROUTE_GET, "https://api.example.com/large-data", 10);
    if (client == NULL) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(500));
        return;
    }

    httprequest_t* req = client->request;
    req->add_header(req, "Accept-Encoding", "gzip");

    httpresponse_t* res = client->send(client);
    if (res->status_code != 200) {
        ctx->response->status_code = res->status_code;
        ctx->response->send_data(ctx->response, "Request failed");
        client->free(client);
        return;
    }

    // get_payload automatically decompresses gzip
    char* body = res->get_payload(res);
    ctx->response->send_data(ctx->response, body ? body : "");

    free(body);
    client->free(client);
}
```

## Client behavior

- **Request headers**: `Host` and `Content-Length` are populated automatically. Add your own via `req->add_header()`.
- **TLS**: for `https://`, certificate verification is enabled by default (minimum TLS version 1.2). See [`set_verify`](#tls-certificate-verification).
- **Redirects**: `301`/`302` are followed automatically, up to 10 hops.
- **Compression**: gzip responses are transparently decompressed when `get_payload()` is called.
- **Keep-alive**: connections with `Connection: keep-alive` are returned to the pool and reused for subsequent requests to the same host/port.
- **Self-invocation**: if the URL targets a domain served by this same server, the request is dispatched through internal route resolution (instead of a network round-trip), including middleware execution.

## Available Methods

| Constant | HTTP Method |
|----------|-------------|
| `ROUTE_GET` | GET |
| `ROUTE_POST` | POST |
| `ROUTE_PUT` | PUT |
| `ROUTE_PATCH` | PATCH |
| `ROUTE_DELETE` | DELETE |
| `ROUTE_HEAD` | HEAD |
| `ROUTE_OPTIONS` | OPTIONS |

## Client methods

```c
// Send the request (always returns the response object; 500 on error)
httpresponse_t* res = client->send(client);

// Reconfigure before sending
client->set_method(client, ROUTE_POST);
client->set_url(client, "https://api.example.com/v2/data");
client->set_verify(client, 1, "/path/to/ca.pem");

// Release
client->free(client);
```

## Request methods

```c
httprequest_t* req = client->request;

// Headers
req->add_header(req, "Header-Name", "value");
req->add_headern(req, "Header-Name", name_len, "value", value_len);
req->remove_header(req, "Header-Name");

// URL-encoded form
req->append_urlencoded(req, "key", "value");

// Multipart form-data
req->append_formdata_text(req, "field", "text value");
req->append_formdata_raw(req, "field", "content/type", "raw data");
req->append_formdata_json(req, "field", json_doc);
req->append_formdata_file(req, "field", &file);            // file_t
req->append_formdata_filepath(req, "field", "/path/file"); // filesystem path

// Request body
req->set_payload_text(req, "plain text");
req->set_payload_raw(req, "content/type", length, "raw data");
req->set_payload_json(req, json_doc);
req->set_payload_file(req, &file);                          // file_t
req->set_payload_filepath(req, "/path/file");               // filesystem path
```

## Response methods

```c
httpresponse_t* res = client->response;

// Response status
int status = res->status_code;

// Body as a string (malloc — free with free(); gzip is decompressed)
char* body = res->get_payload(res);

// Body as JSON (free with json_free())
json_doc_t* doc = res->get_payload_json(res);

// Response header by name
http_header_t* header = res->get_header(res, "Content-Type");
```
