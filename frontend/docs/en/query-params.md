---
outline: deep
description: Query parameter parsing in C Web Framework. Extracting strings, numbers, arrays and objects from the URL through a typed API.
---

# Query Parameters

Query parameters (the part of the URL after `?`) are the most common way to pass small pieces of data to a handler: filters, page numbers, IDs, sort order. The framework parses them once while reading the request and exposes a typed API to retrieve values safely.

Query parameters are available in a handler as a `query_t*` linked list through `ctx->request->query_`. Every accessor takes it as the first argument, the parameter name as the second, and a success flag `int* ok` as the third.

## The query_t structure

Each parameter is a node in a linked list (defined in `query.h`):

```c
typedef struct url_query {
    const char* key;     // parameter name
    const char* value;   // value (URL-decoded)
    struct url_query* next;
} query_t;
```

| Field | Description |
|-------|-------------|
| `key` | Parameter name. Uniqueness is not enforced — on duplicates the first match wins. |
| `value` | Parameter value. **URL-decoded** by the parser automatically (`%20` → space, `+` → space). |
| `next` | Pointer to the next node (`NULL` on the last one). |

::: tip Parser behavior
- String format: `key1=value1&key2=value2&key3`.
- A valueless parameter (`?flag`) is stored as `{key:"flag", value:""}`.
- Anything after `#` is treated as a fragment and is not parsed.
- Keys and values are URL-decoded.
:::

## Function overview

| Function | Returns | Purpose |
|----------|---------|---------|
| `query_param_char` | `const char*` | Value as a string |
| `query_param_int` | `int` | Signed integer |
| `query_param_uint` | `unsigned int` | Unsigned integer |
| `query_param_long` | `long` | Signed `long` |
| `query_param_ulong` | `unsigned long` | Unsigned `long` |
| `query_param_float` | `float` | Single precision |
| `query_param_double` | `double` | Double precision |
| `query_param_ldouble` | `long double` | Extended precision |
| `query_param_array` | `json_doc_t*` | JSON array from the parameter value |
| `query_param_object` | `json_doc_t*` | JSON object from the parameter value |

All functions take `(query_t* query, const char* param_name, int* ok)`. The `ok` flag is set to `1` on success and to `0` when the parameter is missing or the value cannot be parsed. It is idiomatic to initialize the flag to `0` before the call.

## String parameter

```c
#include "query.h"

const char* query_param_char(query_t* query, const char* param_name, int* ok);
```

Returns the parameter value as a string. When a parameter with that key is present, `ok` is `1` even if the value is empty (`?flag` → returns `""`, `ok = 1`). When the key is absent, it returns `NULL` and `ok = 0`.

```c
void handler(httpctx_t* ctx) {
    int ok = 0;
    const char* search = query_param_char(ctx->request->query_, "q", &ok);

    if (!ok || search == NULL) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    ctx->response->send_data(ctx->response, search);
}
```

::: tip Distinguish "missing" from "empty"
`ok = 1` only means the key is present. To tell `?q` (empty value) apart from a missing parameter, additionally check `value == NULL` or `value[0] == 0` when an empty value is not acceptable.
:::

## Integer parameters

```c
int           query_param_int(query_t* query, const char* param_name, int* ok);
unsigned int  query_param_uint(query_t* query, const char* param_name, int* ok);
long          query_param_long(query_t* query, const char* param_name, int* ok);
unsigned long query_param_ulong(query_t* query, const char* param_name, int* ok);
```

Returns the number, or `0` on error. `ok = 0` when the parameter is missing or the value is not a valid integer (empty string, letters, a sign on an unsigned type, etc.). Always check `ok` before relying on the result — `0` can be a legitimate value.

```c
int ok = 0;
int page = query_param_int(ctx->request->query_, "page", &ok);
if (!ok) page = 1;  // default value
```

## Floating-point parameters

```c
float       query_param_float(query_t* query, const char* param_name, int* ok);
double      query_param_double(query_t* query, const char* param_name, int* ok);
long double query_param_ldouble(query_t* query, const char* param_name, int* ok);
```

Returns the number, or `0.0` on error. `ok = 0` when the parameter is missing or the value cannot be converted by `strtof` / `strtod` / `strtold`.

```c
int ok = 0;
double price = query_param_double(ctx->request->query_, "price", &ok);
if (!ok) {
    ctx->response->send_data(ctx->response, "Invalid price");
    return;
}
```

## Array

```c
json_doc_t* query_param_array(query_t* query, const char* param_name, int* ok);
```

Parses the parameter **value** as a JSON array. The parser URL-decodes the value, passes it to `json_parse()`, and verifies the root is an array.

The value must be a valid JSON array:

```
?tags=["php","javascript","python"]
```

Because the value contains special characters, the client must URL-encode it:

```
?tags=%5B%22php%22%2C%22javascript%22%2C%22python%22%5D
```

**Return value** — a `json_doc_t*` containing the array, or `NULL`. The document must be freed with `json_free()`. `ok = 0` when the parameter is missing, the value is empty, or it is not a JSON array.

::: warning Only JSON values are supported
The PHP-style `?tags[]=a&tags[]=b` and the comma-separated `?tags=a,b,c` formats are **not** supported — the value must be a valid JSON array. To iterate elements, use `json_array_size()` and `json_array_get()`.
:::

```c
// URL: /filter?tags=["php","javascript","python"]

void filter_by_tags(httpctx_t* ctx) {
    int ok = 0;
    json_doc_t* tags_doc = query_param_array(ctx->request->query_, "tags", &ok);
    if (!ok || tags_doc == NULL) {
        ctx->response->send_data(ctx->response, "No tags provided");
        return;
    }

    json_token_t* tags = json_root(tags_doc);
    int count = json_array_size(tags);

    str_t* result = str_create_empty(256);
    str_append(result, "Tags: ", 6);

    for (int i = 0; i < count; i++) {
        json_token_t* tag = json_array_get(tags, i);
        const char* tag_value = json_string(tag);

        if (i > 0) str_append(result, ", ", 2);
        str_append(result, tag_value, strlen(tag_value));
    }

    ctx->response->send_data(ctx->response, str_get(result));

    str_free(result);
    json_free(tags_doc);
}
```

## Object

```c
json_doc_t* query_param_object(query_t* query, const char* param_name, int* ok);
```

Parses the parameter **value** as a JSON object. Works like `query_param_array`, but the root must be an object.

The value must be a valid JSON object:

```
?filter={"status":"active","role":"admin"}
```

URL-encoded form:

```
?filter=%7B%22status%22%3A%22active%22%2C%22role%22%3A%22admin%22%7D
```

**Return value** — a `json_doc_t*` containing the object, or `NULL`. The document must be freed with `json_free()`. `ok = 0` when the parameter is missing, the value is empty, or it is not a JSON object.

::: warning Only JSON values are supported
The bracket syntax `?filter[name]=John&filter[age]=25` is **not** supported — the value must be a valid JSON object. To access fields, use `json_object_get()`.
:::

```c
// URL: /users?filter={"status":"active","role":"admin","min_age":18}

void filter_users(httpctx_t* ctx) {
    int ok = 0;
    json_doc_t* filter_doc = query_param_object(ctx->request->query_, "filter", &ok);
    if (!ok || filter_doc == NULL) {
        ctx->response->send_data(ctx->response, "No filter provided");
        return;
    }

    json_token_t* filter = json_root(filter_doc);

    json_token_t* status  = json_object_get(filter, "status");
    json_token_t* role    = json_object_get(filter, "role");
    json_token_t* min_age = json_object_get(filter, "min_age");

    char response[256];
    snprintf(response, sizeof(response),
        "Filter: status=%s, role=%s, min_age=%s",
        status  ? json_string(status)  : "any",
        role    ? json_string(role)    : "any",
        min_age ? json_string(min_age) : "0"
    );

    ctx->response->send_data(ctx->response, response);

    json_free(filter_doc);
}
```

## Usage examples

### Pagination and sorting

```c
// URL: /items?page=2&per_page=20&sort=created_at&order=desc

void list_items(httpctx_t* ctx) {
    int ok = 0;

    int page = query_param_int(ctx->request->query_, "page", &ok);
    if (!ok || page < 1) page = 1;

    int per_page = query_param_int(ctx->request->query_, "per_page", &ok);
    if (!ok || per_page < 1 || per_page > 100) per_page = 20;

    int offset = (page - 1) * per_page;

    const char* sort = query_param_char(ctx->request->query_, "sort", &ok);
    if (!ok || sort == NULL) sort = "created_at";

    const char* order = query_param_char(ctx->request->query_, "order", &ok);
    if (!ok || order == NULL) order = "desc";

    char response[256];
    snprintf(response, sizeof(response),
        "Page: %d, Per page: %d, Offset: %d, Sort: %s %s",
        page, per_page, offset, sort, order
    );

    ctx->response->send_data(ctx->response, response);
}
```

### Multiple filters

```c
// URL: /products?category=electronics&min_price=100&max_price=1000&in_stock=true

void filter_products(httpctx_t* ctx) {
    int ok = 0;

    const char* category = query_param_char(ctx->request->query_, "category", &ok);

    double min_price = query_param_double(ctx->request->query_, "min_price", &ok);
    if (!ok) min_price = 0;

    double max_price = query_param_double(ctx->request->query_, "max_price", &ok);
    if (!ok) max_price = 999999;

    const char* in_stock_str = query_param_char(ctx->request->query_, "in_stock", &ok);
    int in_stock = (ok && in_stock_str && strcmp(in_stock_str, "true") == 0);

    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);

    if (category) json_object_set(root, "category", json_create_string(category));
    json_object_set(root, "min_price", json_create_number(min_price));
    json_object_set(root, "max_price", json_create_number(max_price));
    json_object_set(root, "in_stock", json_create_bool(in_stock));

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(doc));

    json_free(doc);
}
```

### Required numeric ID

```c
// URL: /product?id=42

void get_product(httpctx_t* ctx) {
    int ok = 0;

    int id = query_param_int(ctx->request->query_, "id", &ok);
    if (!ok || id <= 0) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Valid product ID is required");
        return;
    }

    char response[64];
    snprintf(response, sizeof(response), "Product ID: %d", id);
    ctx->response->send_data(ctx->response, response);
}
```

## Validating presence with middleware

The codebase ships a ready-made middleware, `middleware_http_query_param_required`, that verifies a set of required parameters are present and non-empty:

```c
int middleware_http_query_param_required(httpctx_t* ctx, char** keys, int size);
```

It returns `1` (allow) when every key is found and its value is non-empty; otherwise it sends a response naming the missing parameter and returns `0` (reject). It is convenient to call through the `middleware()` macro together with `args_str()`:

```c
#include "httpmiddlewares.h"

void userviewget(httpctx_t* ctx) {
    // Aborts the handler if the "id" parameter is missing or empty
    middleware(
        middleware_http_query_param_required(ctx, args_str("id"))
    )

    int param_ok = 0;
    const int user_id = query_param_int(ctx->request->query_, "id", &param_ok);
    if (!param_ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    // Further processing...
}
```

`args_str(...)` expands to a `(char*[]){...}, count` pair, so the list of keys is written directly in the call. The `middleware(...)` macro runs the check and returns early from the handler (`return`) if it fails.

## Error handling

The rule is simple: always check the `ok` flag right after extracting a parameter. For numbers this is mandatory — `0` can be both a valid value and an error indicator:

```c
void safe_handler(httpctx_t* ctx) {
    int ok = 0;

    int value = query_param_int(ctx->request->query_, "value", &ok);
    if (!ok) {
        // Parameter is missing or has an invalid format
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Invalid or missing 'value' parameter");
        return;
    }

    // Parameter retrieved successfully — value is valid
}
```

::: tip Memory
- `query_param_char` returns a pointer owned by the request — do not free it.
- `query_param_array` and `query_param_object` return a fresh `json_doc_t*` — you must free it with `json_free()`.
:::
