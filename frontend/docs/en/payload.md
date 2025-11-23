---
outline: deep
description: Extracting data from HTTP requests in C Web Framework. Working with multipart/form-data, x-www-form-urlencoded, JSON and file uploads.
---

# Getting data from the client

The framework provides methods for extracting data from HTTP request bodies. Supported formats include `multipart/form-data`, `application/x-www-form-urlencoded`, and `application/json`.

## get_payload

Extracts the entire request body as a string.

```c
#include "http.h"

void post_handler(httpctx_t* ctx) {
    char* payload = ctx->request->get_payload(ctx->request);

    if (!payload) {
        ctx->response->send_data(ctx->response, "Empty payload");
        return;
    }

    ctx->response->send_data(ctx->response, payload);
    free(payload);
}
```

> Returns a pointer to dynamically allocated memory. Always free memory using `free()`.

## get_payloadf

Extracts a field value by key. Works with `multipart/form-data` and `application/x-www-form-urlencoded`.

```c
void post_form(httpctx_t* ctx) {
    char* email = ctx->request->get_payloadf(ctx->request, "email");
    char* password = ctx->request->get_payloadf(ctx->request, "password");

    if (!email || !password) {
        ctx->response->send_data(ctx->response, "Missing required fields");
        if (email) free(email);
        if (password) free(password);
        return;
    }

    // Process data...

    free(email);
    free(password);
    ctx->response->send_data(ctx->response, "Form processed");
}
```

## get_payload_file

Extracts an uploaded file from the request body.

```c
void upload_handler(httpctx_t* ctx) {
    file_content_t file = ctx->request->get_payload_file(ctx->request);

    if (!file.ok) {
        ctx->response->send_data(ctx->response, "No file uploaded");
        return;
    }

    // Save file
    if (!file.save(&file, "uploads", "uploaded_file.txt")) {
        ctx->response->send_data(ctx->response, "Error saving file");
        return;
    }

    // Read file content
    char* content = file.read(&file);
    if (content) {
        ctx->response->send_data(ctx->response, content);
        free(content);
    }
}
```

### file_content_t structure

| Field/Method | Description |
|--------------|-------------|
| `ok` | Successful extraction flag |
| `filename` | Filename from request |
| `save(file, dir, name)` | Save to directory |
| `read(file)` | Read content |

## get_payload_filef

Extracts a file by key from `multipart/form-data`.

```c
void upload_form(httpctx_t* ctx) {
    file_content_t avatar = ctx->request->get_payload_filef(ctx->request, "avatar");

    if (!avatar.ok) {
        ctx->response->send_data(ctx->response, "Avatar not provided");
        return;
    }

    // Save with original filename
    if (!avatar.save(&avatar, "avatars", avatar.filename)) {
        ctx->response->send_data(ctx->response, "Error saving avatar");
        return;
    }

    ctx->response->send_data(ctx->response, "Avatar uploaded");
}
```

## get_payload_json

Parses the request body as JSON and returns a document.

```c
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
    json_token_t* age = json_object_get(root, "age");

    if (name && json_is_string(name)) {
        printf("Name: %s\n", json_string(name));
    }

    if (age && json_is_int(age)) {
        printf("Age: %d\n", json_int(age));
    }

    // Modify and send back
    json_object_set(root, "status", json_create_string(doc, "processed"));

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(doc));

    json_free(doc);
}
```

## get_payload_jsonf

Extracts JSON from a `multipart/form-data` field by key.

```c
void multipart_json(httpctx_t* ctx) {
    json_doc_t* metadata = ctx->request->get_payload_jsonf(ctx->request, "metadata");

    if (!json_ok(metadata)) {
        ctx->response->send_data(ctx->response, "Invalid metadata JSON");
        json_free(metadata);
        return;
    }

    json_token_t* root = json_root(metadata);
    json_token_t* title = json_object_get(root, "title");

    if (title && json_is_string(title)) {
        printf("Title: %s\n", json_string(title));
    }

    json_free(metadata);
    ctx->response->send_data(ctx->response, "Metadata processed");
}
```

## Working with Storage

Saving an uploaded file to storage:

```c
#include "storage.h"

void upload_to_storage(httpctx_t* ctx) {
    file_content_t file = ctx->request->get_payload_filef(ctx->request, "document");

    if (!file.ok) {
        ctx->response->send_data(ctx->response, "No file");
        return;
    }

    // Save to S3-compatible storage
    if (!storage_file_content_put("remote", &file, "documents/%s", file.filename)) {
        ctx->response->send_data(ctx->response, "Storage error");
        return;
    }

    ctx->response->send_data(ctx->response, "File uploaded to storage");
}
```

## Important

- All `payload*` methods return dynamically allocated memory
- Always free memory using `free()` or `json_free()`
- Check the result for `NULL` or `ok` before using
- Maximum request body size is limited by the `client_max_body_size` parameter in configuration
