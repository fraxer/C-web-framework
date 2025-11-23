---
outline: deep
description: HTTP client in C Web Framework. Sending GET, POST requests, working with headers, forms and JSON.
---

# HTTP Client

The framework includes a built-in HTTP client for making outgoing HTTP requests to external services.

## Initialization

```c
#include "httpclient.h"

httpclient_t* httpclient_init(int method, const char* url, int timeout);
```

Creates a new HTTP client.

**Parameters**\
`method` — HTTP method (`ROUTE_GET`, `ROUTE_POST`, `ROUTE_PUT`, `ROUTE_DELETE`, etc.).\
`url` — URL for the request.\
`timeout` — timeout in seconds.

**Return Value**\
Pointer to the client or `NULL` on error.

## Sending a Request

```c
httpresponse_t* client->send(httpclient_t* client);
```

Executes an HTTP request.

**Return Value**\
Pointer to the response or `NULL` on connection error.

## Freeing Resources

```c
client->free(httpclient_t* client);
```

Frees client memory and related objects.

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
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "Failed to create client");
        return;
    }

    // Add headers
    httprequest_t* req = client->request;
    req->add_header(req, "Accept", "application/json");
    req->add_header(req, "User-Agent", "C-Web-Framework/1.0");

    // Send request
    httpresponse_t* res = client->send(client);
    if (res == NULL) {
        ctx->response->status_code = 502;
        ctx->response->send_data(ctx->response, "Connection failed");
        client->free(client);
        return;
    }

    // Check status
    if (res->status_code != 200) {
        ctx->response->status_code = res->status_code;
        ctx->response->send_data(ctx->response, "External API error");
        client->free(client);
        return;
    }

    // Get response body
    char* body = res->get_payload(res);
    if (body == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "Failed to read response");
        client->free(client);
        return;
    }

    // Send response to client
    ctx->response->header_add(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, body);

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
        ctx->response->send_data(ctx->response, "Client error");
        return;
    }

    httprequest_t* req = client->request;

    // Add form parameters
    req->append_urlencoded(req, "username", "john");
    req->append_urlencoded(req, "password", "secret123");

    httpresponse_t* res = client->send(client);
    if (res == NULL || res->status_code != 200) {
        ctx->response->send_data(ctx->response, "Request failed");
        client->free(client);
        return;
    }

    char* body = res->get_payload(res);
    ctx->response->send_data(ctx->response, body);

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
        ctx->response->send_data(ctx->response, "Client error");
        return;
    }

    httprequest_t* req = client->request;

    // Create JSON document
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);
    json_object_set(root, "name", json_create_string("John Doe"));
    json_object_set(root, "email", json_create_string("john@example.com"));
    json_object_set(root, "age", json_create_number(30));

    // Set JSON as request body
    req->set_payload_json(req, doc);
    json_free(doc);

    httpresponse_t* res = client->send(client);
    if (res == NULL) {
        ctx->response->send_data(ctx->response, "Connection failed");
        client->free(client);
        return;
    }

    char* body = res->get_payload(res);

    ctx->response->status_code = res->status_code;
    ctx->response->header_add(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, body ? body : "{}");

    if (body) free(body);
    client->free(client);
}
```

### POST with multipart/form-data

```c
void post_formdata(httpctx_t* ctx) {
    const char* url = "https://api.example.com/upload";
    httpclient_t* client = httpclient_init(ROUTE_POST, url, 10);

    if (client == NULL) {
        ctx->response->send_data(ctx->response, "Client error");
        return;
    }

    httprequest_t* req = client->request;

    // Text field
    req->append_formdata_text(req, "description", "My file description");

    // Raw data with type specification
    req->append_formdata_raw(req, "metadata", "{\"type\":\"image\"}", "application/json");

    // JSON object
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);
    json_object_set(root, "tags", json_create_string("photo,vacation"));
    req->append_formdata_json(req, "options", doc);
    json_free(doc);

    // File from storage
    file_t file = storage_file_get("local", "uploads/image.png");
    if (file.ok) {
        req->append_formdata_file(req, "file", &file);
        file.close(&file);
    }

    httpresponse_t* res = client->send(client);
    if (res == NULL) {
        ctx->response->send_data(ctx->response, "Upload failed");
        client->free(client);
        return;
    }

    char* body = res->get_payload(res);
    ctx->response->send_data(ctx->response, body ? body : "Success");

    if (body) free(body);
    client->free(client);
}
```

### Request with Authorization

```c
void authorized_request(httpctx_t* ctx) {
    const char* url = "https://api.example.com/protected";
    httpclient_t* client = httpclient_init(ROUTE_GET, url, 5);

    if (client == NULL) {
        ctx->response->send_data(ctx->response, "Client error");
        return;
    }

    httprequest_t* req = client->request;

    // Bearer token
    req->add_header(req, "Authorization", "Bearer your-jwt-token-here");

    // Or Basic authorization
    // req->add_header(req, "Authorization", "Basic dXNlcjpwYXNz");

    httpresponse_t* res = client->send(client);

    if (res == NULL) {
        ctx->response->send_data(ctx->response, "Connection failed");
        client->free(client);
        return;
    }

    if (res->status_code == 401) {
        ctx->response->status_code = 401;
        ctx->response->send_data(ctx->response, "Unauthorized");
        client->free(client);
        return;
    }

    char* body = res->get_payload(res);
    ctx->response->send_data(ctx->response, body);

    free(body);
    client->free(client);
}
```

### Handling gzip Response

```c
void fetch_compressed(httpctx_t* ctx) {
    const char* url = "https://api.example.com/large-data";
    httpclient_t* client = httpclient_init(ROUTE_GET, url, 10);

    if (client == NULL) {
        ctx->response->send_data(ctx->response, "Client error");
        return;
    }

    httprequest_t* req = client->request;

    // Request compressed response
    req->add_header(req, "Accept-Encoding", "gzip");

    httpresponse_t* res = client->send(client);
    if (res == NULL || res->status_code != 200) {
        ctx->response->send_data(ctx->response, "Request failed");
        client->free(client);
        return;
    }

    // get_payload automatically decompresses gzip
    char* body = res->get_payload(res);
    ctx->response->send_data(ctx->response, body);

    free(body);
    client->free(client);
}
```

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

## Request Methods

```c
// Add header
req->add_header(req, "Header-Name", "value");

// URL-encoded data
req->append_urlencoded(req, "key", "value");

// Multipart form-data
req->append_formdata_text(req, "field", "text value");
req->append_formdata_raw(req, "field", "raw data", "content/type");
req->append_formdata_json(req, "field", json_doc);
req->append_formdata_file(req, "field", &file);

// JSON body
req->set_payload_json(req, json_doc);
```

## Response Methods

```c
// Response status
int status = res->status_code;

// Response body (must be freed with free())
char* body = res->get_payload(res);
```
