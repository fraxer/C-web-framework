---
outline: deep
description: Query parameter parsing in C Web Framework. Extracting strings, numbers, arrays and objects from URL.
---

# Query Parameters

The framework provides a typed API for extracting query parameters from the request URL.

## API

### String Parameter

```c
#include "query.h"

const char* query_param_char(httprequest_t* request, const char* param_name, int* ok);
```

Gets a parameter as a string.

**Parameters**\
`request` — pointer to HTTP request.\
`param_name` — parameter name.\
`ok` — pointer to success flag (1 — success, 0 — error).

**Return Value**\
Pointer to the string value or `NULL`.

<br>

### Integer Parameters

```c
int query_param_int(httprequest_t* request, const char* param_name, int* ok);
unsigned int query_param_uint(httprequest_t* request, const char* param_name, int* ok);
long query_param_long(httprequest_t* request, const char* param_name, int* ok);
unsigned long query_param_ulong(httprequest_t* request, const char* param_name, int* ok);
```

**Return Value**\
Numeric value or 0 on error (check the `ok` flag).

<br>

### Floating-Point Parameters

```c
float query_param_float(httprequest_t* request, const char* param_name, int* ok);
double query_param_double(httprequest_t* request, const char* param_name, int* ok);
long double query_param_ldouble(httprequest_t* request, const char* param_name, int* ok);
```

**Return Value**\
Numeric value or 0.0 on error.

<br>

### Array

```c
json_doc_t* query_param_array(httprequest_t* request, const char* param_name, int* ok);
```

Parses an array from the query string.

Supported formats:
- `?items[]=a&items[]=b&items[]=c`
- `?items=a,b,c`

**Return Value**\
JSON document with the array. Must be freed with `json_free()`.

<br>

### Object

```c
json_doc_t* query_param_object(httprequest_t* request, const char* param_name, int* ok);
```

Parses an object from the query string.

Format: `?filter[name]=John&filter[age]=25`

**Return Value**\
JSON document with the object. Must be freed with `json_free()`.

## Usage Examples

### Basic Parameters

```c
// URL: /search?query=hello&page=2&limit=10

void search(httpctx_t* ctx) {
    int ok = 0;

    // String parameter
    const char* query = query_param_char(ctx->request, "query", &ok);
    if (!ok || query == NULL) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Parameter 'query' is required");
        return;
    }

    // Numeric parameters with default values
    int page = query_param_int(ctx->request, "page", &ok);
    if (!ok) page = 1;

    int limit = query_param_int(ctx->request, "limit", &ok);
    if (!ok) limit = 20;

    char response[256];
    snprintf(response, sizeof(response),
        "Search: %s, Page: %d, Limit: %d",
        query, page, limit
    );

    ctx->response->send_data(ctx->response, response);
}
```

### Floating-Point Parameters

```c
// URL: /calculate?price=99.99&tax=0.2

void calculate(httpctx_t* ctx) {
    int ok = 0;

    double price = query_param_double(ctx->request, "price", &ok);
    if (!ok) {
        ctx->response->send_data(ctx->response, "Invalid price");
        return;
    }

    double tax = query_param_double(ctx->request, "tax", &ok);
    if (!ok) tax = 0.0;

    double total = price * (1 + tax);

    char response[64];
    snprintf(response, sizeof(response), "Total: %.2f", total);
    ctx->response->send_data(ctx->response, response);
}
```

### Working with Arrays

```c
// URL: /filter?tags[]=php&tags[]=javascript&tags[]=python
// or: /filter?tags=php,javascript,python

void filter_by_tags(httpctx_t* ctx) {
    int ok = 0;

    json_doc_t* tags_doc = query_param_array(ctx->request, "tags", &ok);
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

### Working with Objects

```c
// URL: /users?filter[status]=active&filter[role]=admin&filter[min_age]=18

void filter_users(httpctx_t* ctx) {
    int ok = 0;

    json_doc_t* filter_doc = query_param_object(ctx->request, "filter", &ok);
    if (!ok || filter_doc == NULL) {
        ctx->response->send_data(ctx->response, "No filter provided");
        return;
    }

    json_token_t* filter = json_root(filter_doc);

    // Extract values from object
    json_token_t* status_token = json_object_get(filter, "status");
    json_token_t* role_token = json_object_get(filter, "role");
    json_token_t* min_age_token = json_object_get(filter, "min_age");

    const char* status = status_token ? json_string(status_token) : "any";
    const char* role = role_token ? json_string(role_token) : "any";
    int min_age = min_age_token ? atoi(json_string(min_age_token)) : 0;

    char response[256];
    snprintf(response, sizeof(response),
        "Filter: status=%s, role=%s, min_age=%d",
        status, role, min_age
    );

    ctx->response->send_data(ctx->response, response);

    json_free(filter_doc);
}
```

### Parameter Validation

```c
void get_product(httpctx_t* ctx) {
    int ok = 0;

    // ID is required and must be positive
    int id = query_param_int(ctx->request, "id", &ok);
    if (!ok || id <= 0) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Valid product ID is required");
        return;
    }

    // Get product...
    char response[64];
    snprintf(response, sizeof(response), "Product ID: %d", id);
    ctx->response->send_data(ctx->response, response);
}
```

### Pagination

```c
void list_items(httpctx_t* ctx) {
    int ok = 0;

    // Pagination parameters with default values
    int page = query_param_int(ctx->request, "page", &ok);
    if (!ok || page < 1) page = 1;

    int per_page = query_param_int(ctx->request, "per_page", &ok);
    if (!ok || per_page < 1 || per_page > 100) per_page = 20;

    int offset = (page - 1) * per_page;

    // Sorting
    const char* sort = query_param_char(ctx->request, "sort", &ok);
    if (!ok || sort == NULL) sort = "created_at";

    const char* order = query_param_char(ctx->request, "order", &ok);
    if (!ok || order == NULL) order = "desc";

    // Build SQL query or use ORM...
    char response[256];
    snprintf(response, sizeof(response),
        "Page: %d, Per page: %d, Offset: %d, Sort: %s %s",
        page, per_page, offset, sort, order
    );

    ctx->response->send_data(ctx->response, response);
}
```

### Multiple Filters

```c
// URL: /products?category=electronics&min_price=100&max_price=1000&in_stock=true

void filter_products(httpctx_t* ctx) {
    int ok = 0;

    const char* category = query_param_char(ctx->request, "category", &ok);

    double min_price = query_param_double(ctx->request, "min_price", &ok);
    if (!ok) min_price = 0;

    double max_price = query_param_double(ctx->request, "max_price", &ok);
    if (!ok) max_price = 999999;

    const char* in_stock_str = query_param_char(ctx->request, "in_stock", &ok);
    int in_stock = (ok && in_stock_str && strcmp(in_stock_str, "true") == 0);

    // Apply filters...
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);

    if (category) json_object_set(root, "category", json_create_string(category));
    json_object_set(root, "min_price", json_create_number(min_price));
    json_object_set(root, "max_price", json_create_number(max_price));
    json_object_set(root, "in_stock", json_create_bool(in_stock));

    ctx->response->header_add(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(doc));

    json_free(doc);
}
```

## Error Handling

Always check the `ok` flag after extracting a parameter:

```c
void safe_handler(httpctx_t* ctx) {
    int ok = 0;

    int value = query_param_int(ctx->request, "value", &ok);

    if (!ok) {
        // Parameter is missing or has invalid format
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Invalid or missing 'value' parameter");
        return;
    }

    // Parameter successfully retrieved
    // ...
}
```
