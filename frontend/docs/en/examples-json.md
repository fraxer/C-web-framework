---
outline: deep
description: Examples of working with json format
---

# JSON examples

Ready-to-use examples for parsing, building, and serializing JSON documents.
For the full function reference, see [JSON](/en/json).

## Parsing a string

`json_parse` takes a string and returns a ready-to-use document. The root token
is available through `json_root`, and its type is checked with `json_is_*`:

```c
// handlers/jsonpage.c
#include "http.h"
#include "json.h"

void parse_json(httpctx_t* ctx) {
    const char* json_string =
        "{\"name\":\"alex\",\"age\":30,\"active\":true,\"balance\":165.23}";

    json_doc_t* doc = json_parse(json_string);
    if (!doc) {
        ctx->response->send_data(ctx->response, "json parse error");
        return;
    }

    json_token_t* object = json_root(doc);
    if (!json_is_object(object)) {
        ctx->response->send_data(ctx->response, "root is not an object");
        json_free(doc);
        return;
    }

    json_token_t* name = json_object_get(object, "name");
    if (name) {
        ctx->response->send_data(ctx->response, json_string(name));
    }

    json_free(doc);
}
```

## Building a document from scratch

A convenient entry point is `json_root_create_object` (or `json_root_create_array`),
which creates the document and assigns the root token in one call. Tokens are
created without a document: `json_create_string`, `json_create_number`,
`json_create_bool`, `json_create_null`, `json_create_object`, `json_create_array`:

```c
#include "http.h"
#include "json.h"

void build_json(httpctx_t* ctx) {
    json_doc_t* doc = json_root_create_object();
    json_token_t* object = json_root(doc);

    json_object_set(object, "name", json_create_string("alex"));
    json_object_set(object, "age", json_create_number(30));
    json_object_set(object, "active", json_create_bool(1));
    json_object_set(object, "balance", json_create_number(165.23));
    json_object_set(object, "note", json_create_null());

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(doc));

    json_free(doc);
}
```

You can also start from an empty document and assign the root separately with
`json_set_root`:

```c
json_doc_t* doc = json_create_empty();
json_token_t* object = json_create_object();
json_set_root(doc, object);
```

## Nested objects and arrays

Container tokens can be freely nested. Create the values first, then pass them
to `json_object_set` / `json_array_append`:

```c
#include "http.h"
#include "json.h"

void nested_json(httpctx_t* ctx) {
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);

    // Nested "address" object
    json_token_t* address = json_create_object();
    json_object_set(address, "city", json_create_string("Berlin"));
    json_object_set(address, "zip", json_create_number(10115));
    json_object_set(root, "address", address);

    // "tags" array with prepend/append
    json_token_t* tags = json_create_array();
    json_object_set(root, "tags", tags);

    json_array_append(tags, json_create_string("php"));
    json_array_append(tags, json_create_string("c"));
    json_array_prepend(tags, json_create_string("first"));
    json_array_append_to(tags, 1, json_create_string("sql"));  // insert at index

    // Replace a value by index
    json_token_t* second = json_array_get(tags, 1);
    if (second) {
        json_token_set_string(second, "rust");
    }

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(doc));

    json_free(doc);
}
```

## Reading typed values

The numeric getters `json_int`, `json_uint`, `json_llong`, and `json_double`
take an `int* ok` out-parameter — it is zero when the token is not a number.
Use `json_is_number` (and the other `json_is_*` checks) to verify the type:

```c
#include "http.h"
#include "json.h"

void read_numbers(httpctx_t* ctx) {
    json_doc_t* doc = json_parse("{\"count\":42,\"price\":19.95,\"name\":\"alex\"}");
    if (!doc) {
        ctx->response->send_data(ctx->response, "parse error");
        return;
    }

    json_token_t* object = json_root(doc);
    char response[256];

    json_token_t* count = json_object_get(object, "count");
    if (count && json_is_number(count)) {
        int ok = 0;
        int value = json_int(count, &ok);
        if (ok) {
            snprintf(response, sizeof(response), "count = %d", value);
            ctx->response->send_data(ctx->response, response);
        }
    }

    json_free(doc);
}
```

Strings and booleans are read directly — without the `ok` parameter:
`json_string(token)`, `json_bool(token)`. The byte length of a string is
returned by `json_string_size`.

## Iterating an object and an array

The single `json_it_t` iterator works for both objects and arrays. For an
object, the key is available through `json_it_key` and the value through
`json_it_value`:

```c
#include "http.h"
#include "json.h"

void iterate_json(httpctx_t* ctx) {
    json_doc_t* doc = json_parse("{\"a\":1,\"b\":2,\"c\":3}");
    if (!doc) {
        ctx->response->send_data(ctx->response, "parse error");
        return;
    }

    json_token_t* object = json_root(doc);
    char response[512] = "";

    for (json_it_t it = json_init_it(object); !json_end_it(&it); it = json_next_it(&it)) {
        const char* key = (const char*)json_it_key(&it);
        json_token_t* value = json_it_value(&it);

        int ok = 0;
        int num = json_int(value, &ok);

        char line[64];
        snprintf(line, sizeof(line), "%s=%d | ", key, ok ? num : 0);
        strncat(response, line, sizeof(response) - strlen(response) - 1);
    }

    ctx->response->send_data(ctx->response, response);
    json_free(doc);
}
```

When iterating an array, `json_it_key` returns `NULL` — only values are
available. To remove the current element as you go, call `json_it_erase`.

## Serializing and responding

`json_stringify` returns a string owned by the document and freed together
with it via `json_free`. Setting `ascii_mode = 1` encodes every non-ASCII
character as `\uXXXX` — handy for logs and compatibility with legacy clients:

```c
#include "http.h"
#include "json.h"

void stringify_json(httpctx_t* ctx) {
    json_doc_t* doc = json_parse("[\"Hello 😅\", 42, true]");
    if (!doc) {
        ctx->response->send_data(ctx->response, "parse error");
        return;
    }

    doc->ascii_mode = 1;  // escape non-ASCII as \uXXXX

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(doc));

    json_free(doc);
}
```

When the string must outlive the document, use `json_stringify_detach`. It
returns a separate copy that you must release manually with `free`:

```c
char* body = json_stringify_detach(doc);
if (body) {
    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, body);
    free(body);
}
json_free(doc);
```
