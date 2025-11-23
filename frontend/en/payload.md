---
outline: deep
description: Cody introduces methods for extracting data from a request when encoding multipart/form-data and application/x-www-form-urlencoded
---

# Getting data from the client

Cpdy provides several methods for extracting data from a request.

### get_payload

`char*(*get_payload)(httpctx_t*);`

Retrieves all available data from the request body as a string.

Returns a pointer of type `char` to dynamically allocated memory.

After working with the data, be sure to free the memory.

```C
#include "http1.h"

void post(httpctx_t* ctx) {
    char* payload = ctx->request->get_payload(ctx);

    if (payload == NULL) {
        ctx->response->send_data(ctx, "Payload not found");
        return;
    }

    ctx->response->send_data(ctx, payload);

    free(payload);
}
```

### get_payloadf

`char*(*get_payloadf)(httpctx_t*, const char*);`

Retrieves data by key from the request body as a string. Used when encoding `multipart/form-data` and `application/x-www-form-urlencoded` data.

Returns a pointer of type `char` to dynamically allocated memory.

After working with the data, be sure to free the memory.

```C
#include "http1.h"

void post(httpctx_t* ctx) {
    char* data = ctx->request->get_payloadf(ctx, "mydata");
    char* text = ctx->request->get_payloadf(ctx, "mytext");

    if (data == NULL) {
        ctx->response->send_data(ctx, "Data not found");
        goto failed;
    }

    if (text == NULL) {
        ctx->response->send_data(ctx, "Text not found");
        goto failed;
    }

    ctx->response->send_data(ctx, "Payload processed");

    failed:

    if (data) free(data);
    if (text) free(text);
}
```

### get_payload_file

`file_content_t(*get_payload_file)(httpctx_t*);`

Extracts all available data from the request body and wraps it in a `file_content_t` structure.

Returns the `file_content_t` structure. The structure has a `name` field for storing the name of the file, a `save` method for saving the file to a directory with the specified file name, and a `read` method for extracting the contents of the file as a string.

The `read` method allocates dynamic memory to store the string, so you need to deallocate the memory at the pointer.

```C
#include "http1.h"

void post(httpctx_t* ctx) {
    file_content_t payloadfile = ctx->request->get_payload_file(ctx);

    if (!payloadfile.ok) {
        ctx->response->send_data(ctx, "file not found");
        return;
    }

    if (!payloadfile.save(&payloadfile, "files", "file.txt")) {
        ctx->response->send_data(ctx, "Error save file");
        return;
    }

    char* data = payloadfile.read(&payloadfile);

    ctx->response->send_data(ctx, data);

    free(data);
}
```

### get_payload_filef

`file_content_t(*get_payload_filef)(httpctx_t*, const char*);`

Extracts data by key from the request body and wraps it in a `file_content_t` structure. Used when encoding `multipart/form-data` data.

Returns the `file_content_t` structure. The structure has a `name` field for storing the name of the file, a `save` method for saving the file to a directory with the specified file name, and a `read` method for extracting the contents of the file as a string.

The `read` method allocates dynamic memory to store the string, so you need to deallocate the memory at the pointer.

```C
#include "http1.h"

void post(httpctx_t* ctx) {
    file_content_t payloadfile = ctx->request->get_payload_filef(ctx, "myfile");

    if (!payloadfile.ok) {
        ctx->response->send_data(ctx, "file not found");
        return;
    }

    const char* filenameFromPayload = NULL;
    if (!payloadfile.save(&payloadfile, "files", filenameFromPayload)) {
        ctx->response->send_data(ctx, "Error save file");
        return;
    }

    char* data = payloadfile.read(&payloadfile);

    ctx->response->send_data(ctx, data);

    free(data);
}
```

### get_payload_json

`json_doc_t*(*get_payload_json)(httpctx_t*);`

Extracts all available data from the request body and creates a json document `json_doc_t`.

Returns a pointer of type `json_doc_t`.

After working with the json document, you need to free the memory.

```C
#include "http1.h"
#include "json.h"

void post(httpctx_t* ctx) {
    json_doc_t* document = ctx->request->get_payload_json(ctx);

    if (!json_ok(document)) {
        ctx->response->send_data(ctx, json_error(document));
        goto failed;
    }

    json_token_t* object = json_root(document);
    if (!json_is_object(object)) {
        ctx->response->send_data(ctx, "is not object");
        goto failed;
    }

    json_object_set(object, "mykey", json_create_string(document, "Hello"));

    ctx->response->header_add(ctx, "Content-Type", "application/json");

    ctx->response->send_data(ctx, json_stringify(document));

    failed:

    json_free(document);
}
```

### get_payload_jsonf

`json_doc_t*(*get_payload_jsonf)(httpctx_t*, const char*);`

Extracts data by key from the request body and creates a json document `json_doc_t`. Used when encoding `multipart/form-data` data.

Returns a pointer of type `json_doc_t`.

After working with the json document, you need to free the memory.

```C
#include "http1.h"
#include "json.h"

void post(httpctx_t* ctx) {
    json_doc_t* document = ctx->request->get_payload_jsonf(ctx, "myjson");

    if (!json_ok(document)) {
        ctx->response->send_data(ctx, json_error(document));
        goto failed;
    }

    json_token_t* object = json_root(document);
    if (!json_is_object(object)) {
        ctx->response->send_data(ctx, "is not object");
        goto failed;
    }

    json_object_set(object, "mykey", json_create_string(document, "Hello"));

    ctx->response->header_add(ctx, "Content-Type", "application/json");

    ctx->response->send_data(ctx, json_stringify(document));

    failed:

    json_free(document);
}
```

