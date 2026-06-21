---
outline: deep
description: Extracting data from HTTP requests in C Web Framework. Working with multipart/form-data, x-www-form-urlencoded, JSON and file uploads.
---

# Getting data from the client

The framework provides methods for extracting data from the HTTP request body. The body is written to an internal spool file and parsed lazily — on the first access through a `get_payload*` method. Supported formats include `multipart/form-data`, `application/x-www-form-urlencoded`, `application/json`, and arbitrary binary/text bodies.

The methods are attached to the request object and called via `ctx->request-><method>(ctx->request, ...)`.

## Methods overview

| Method | Returns | Purpose | Body formats |
|--------|---------|---------|--------------|
| `get_payload` | `char*` | Entire body as a single string | `PLAIN`, `MULTIPART` |
| `get_payloadf` | `char*` | Field value by key | `MULTIPART`, `URLENCODED` |
| `get_payload_file` | `file_content_t` | File from the body (first part) | `MULTIPART`, `PLAIN` |
| `get_payload_filef` | `file_content_t` | File by key | `MULTIPART` |
| `get_payload_json` | `json_doc_t*` | Body parsed as JSON | any text body |
| `get_payload_jsonf` | `json_doc_t*` | JSON from a field by key | `MULTIPART` |

::: tip Internal body type
When reading the `Content-Type` header, the framework determines the body type (`http_payload_type_e`): `PLAIN` — arbitrary body (including `application/json`), `MULTIPART` — `multipart/form-data`, `URLENCODED` — `application/x-www-form-urlencoded`, `NONE` — no body. Which method fits depends on this type.
:::

## get_payload

`char* (*get_payload)(httprequest_t* request);`

Extracts the entire request body as a string. Equivalent to `get_payloadf(request, NULL)` — returns the first part (`PLAIN`) or the whole `MULTIPART` body. Returns `NULL` for `URLENCODED` (use `get_payloadf` with a key).

```c
#include "http.h"

void post_handler(httpctx_t* ctx) {
    char* payload = ctx->request->get_payload(ctx->request);

    if (payload == NULL) {
        ctx->response->send_data(ctx->response, "Empty payload");
        return;
    }

    ctx->response->send_data(ctx->response, payload);

    free(payload);
}
```

> Returns a pointer to dynamically allocated memory. Always free it with `free()`.

## get_payloadf

`char* (*get_payloadf)(httprequest_t* request, const char* field);`

Extracts a field value by key. Works with `multipart/form-data` and `application/x-www-form-urlencoded`.

```c
#include "http.h"

void post_form(httpctx_t* ctx) {
    char* email = ctx->request->get_payloadf(ctx->request, "email");
    char* password = ctx->request->get_payloadf(ctx->request, "password");

    if (email == NULL || password == NULL) {
        ctx->response->send_data(ctx->response, "Missing required fields");
        goto failed;
    }

    // Process data...

    ctx->response->send_data(ctx->response, "Form processed");

    failed:

    if (email) free(email);
    if (password) free(password);
}
```

## get_payload_file

`file_content_t (*get_payload_file)(httprequest_t* request);`

Extracts an uploaded file from the request body — the first `MULTIPART` part (or the whole body for `PLAIN`). Equivalent to `get_payload_filef(request, NULL)`. Returns a lightweight `file_content_t` that **does not copy** the data — it references an offset into the internal spool file. Copying happens only in `make_file()` or `content()`.

```c
#include "http.h"

void upload_handler(httpctx_t* ctx) {
    file_content_t file = ctx->request->get_payload_file(ctx->request);

    if (!file.ok) {
        ctx->response->send_data(ctx->response, "No file uploaded");
        return;
    }

    // make_file(dir, name) copies the data to disk and returns an open file_t.
    // name = NULL keeps the original file.filename.
    file_t saved = file.make_file(&file, "uploads", "uploaded_file.txt");
    if (!saved.ok) {
        ctx->response->send_data(ctx->response, "Error saving file");
        return;
    }

    // content() reads the bytes into an allocated buffer (freed with free).
    char* content = file.content(&file);
    if (content) {
        ctx->response->send_data(ctx->response, content);
        free(content);
    }

    saved.close(&saved);
}
```

### `file_content_t` structure

| Field / Method | Type | Description |
|----------------|------|-------------|
| `ok` | `int` | Successful extraction flag (`1` = data is valid) |
| `fd` | `int` | File descriptor of the internal spool file |
| `offset` | `off_t` | Offset where the file data starts |
| `size` | `size_t` | Content size in bytes |
| `filename` | `char[NAME_MAX]` | Filename provided by the client |
| `set_filename(name)` | `int` | Set the file name (`1` on success) |
| `make_file(dir, name)` | `file_t` | Save to directory `dir` (`name = NULL` → original name) |
| `make_tmpfile(dir)` | `file_t` | Save to a temporary file in `dir` (deleted on `close()`) |
| `content()` | `char*` | Read the bytes into memory (freed with `free`) |

## get_payload_filef

`file_content_t (*get_payload_filef)(httprequest_t* request, const char* field);`

Extracts a file by key from `multipart/form-data`. If the field is found but contains no file (no `filename`), `ok` will be `0`.

```c
#include "http.h"

void upload_form(httpctx_t* ctx) {
    file_content_t avatar = ctx->request->get_payload_filef(ctx->request, "avatar");

    if (!avatar.ok) {
        ctx->response->send_data(ctx->response, "Avatar not provided");
        return;
    }

    // NULL keeps the original filename from the request.
    file_t saved = avatar.make_file(&avatar, "avatars", NULL);
    if (!saved.ok) {
        ctx->response->send_data(ctx->response, "Error saving avatar");
        return;
    }

    saved.close(&saved);

    ctx->response->send_data(ctx->response, "Avatar uploaded");
}
```

### `file_t` structure

Returned by `make_file()` / `make_tmpfile()`. Main methods:

| Method | Returns | Description |
|--------|---------|-------------|
| `ok` / `tmp` / `fd` | fields | `ok` — success; `tmp=1` — temp file (removed on `close`); `fd` — descriptor |
| `size` / `mtime` / `name` | fields | Size, modification time, file name |
| `content()` | `char*` | Read the contents (freed with `free`) |
| `set_content(data, size)` | `int` | Overwrite the contents |
| `append_content(data, size)` | `int` | Append to the end |
| `truncate(offset)` | `int` | Truncate to the given size |
| `close()` | `int` | Close (and delete if `tmp=1`) |

## get_payload_json

`json_doc_t* (*get_payload_json)(httprequest_t* request);`

Parses the entire request body as JSON and returns a document. Equivalent to `get_payload_jsonf(request, NULL)`. Returns `NULL` on a parse error.

```c
#include "http.h"
#include "json.h"

void post_json(httpctx_t* ctx) {
    json_doc_t* doc = ctx->request->get_payload_json(ctx->request);

    if (!json_ok(doc)) {
        ctx->response->send_data(ctx->response, json_error(doc));
        json_free(doc);
        return;
    }

    json_token_t* root = json_root(doc);

    if (!json_is_object(root)) {
        ctx->response->send_data(ctx->response, "Expected JSON object");
        json_free(doc);
        return;
    }

    // Extract fields
    json_token_t* name = json_object_get(root, "name");
    json_token_t* age  = json_object_get(root, "age");

    if (name && json_is_string(name)) {
        printf("Name: %s\n", json_string(name));
    }

    if (age && json_is_number(age)) {
        int ok = 0;
        printf("Age: %d\n", json_int(age, &ok));
    }

    // Modify and send back
    json_object_set(root, "status", json_create_string("processed"));

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(doc));

    json_free(doc);
}
```

## get_payload_jsonf

`json_doc_t* (*get_payload_jsonf)(httprequest_t* request, const char* field);`

Extracts JSON from a `multipart/form-data` field by key and parses it. Convenient for sending files alongside structured metadata.

```c
#include "http.h"
#include "json.h"

void multipart_json(httpctx_t* ctx) {
    json_doc_t* metadata = ctx->request->get_payload_jsonf(ctx->request, "metadata");

    if (!json_ok(metadata)) {
        ctx->response->send_data(ctx->response, "Invalid metadata JSON");
        json_free(metadata);
        return;
    }

    json_token_t* root  = json_root(metadata);
    json_token_t* title = json_object_get(root, "title");

    if (title && json_is_string(title)) {
        printf("Title: %s\n", json_string(title));
    }

    json_free(metadata);

    ctx->response->send_data(ctx->response, "Metadata processed");
}
```

## Working with Storage

An uploaded file can be sent directly to a configured storage (FS or S3-compatible), bypassing manual copying:

```c
#include "http.h"
#include "storage.h"

void upload_to_storage(httpctx_t* ctx) {
    file_content_t file = ctx->request->get_payload_filef(ctx->request, "document");

    if (!file.ok) {
        ctx->response->send_data(ctx->response, "No file");
        return;
    }

    // Save to the named storage "remote" (see the storages section in config.json).
    if (!storage_file_content_put("remote", &file, "documents/%s", file.filename)) {
        ctx->response->send_data(ctx->response, "Storage error");
        return;
    }

    ctx->response->send_data(ctx->response, "File uploaded to storage");
}
```

## Memory management

- `get_payload` / `get_payloadf` return a dynamically allocated `char*` — free it with `free()`.
- `get_payload_json` / `get_payload_jsonf` return a `json_doc_t*` — free it with `json_free()`.
- The `file_content_t` from `get_payload_file*` **does not own** the data — copying happens only when you call `make_file()` / `content()`. The buffer from `content()` is freed with `free()`, and the `file_t` from `make_file()` is closed with `close()`.
- Always check the result for `NULL` (strings/JSON) or the `ok` field (`file_content_t` / `file_t`) before use.
- The maximum request body size is limited by the `client_max_body_size` parameter in the `main` section of the configuration.
