---
outline: deep
description: Парсинг query-параметров в C Web Framework. Извлечение строк, чисел, массивов и объектов из URL.
---

# Query-параметры

Фреймворк предоставляет типизированный API для извлечения query-параметров из URL запроса.

## API

### Строковый параметр

```c
#include "query.h"

const char* query_param_char(httprequest_t* request, const char* param_name, int* ok);
```

Получает параметр как строку.

**Параметры**\
`request` — указатель на HTTP-запрос.\
`param_name` — имя параметра.\
`ok` — указатель на флаг успеха (1 — успех, 0 — ошибка).

**Возвращаемое значение**\
Указатель на строку со значением или `NULL`.

<br>

### Целочисленные параметры

```c
int query_param_int(httprequest_t* request, const char* param_name, int* ok);
unsigned int query_param_uint(httprequest_t* request, const char* param_name, int* ok);
long query_param_long(httprequest_t* request, const char* param_name, int* ok);
unsigned long query_param_ulong(httprequest_t* request, const char* param_name, int* ok);
```

**Возвращаемое значение**\
Числовое значение или 0 при ошибке (проверяйте флаг `ok`).

<br>

### Параметры с плавающей точкой

```c
float query_param_float(httprequest_t* request, const char* param_name, int* ok);
double query_param_double(httprequest_t* request, const char* param_name, int* ok);
long double query_param_ldouble(httprequest_t* request, const char* param_name, int* ok);
```

**Возвращаемое значение**\
Числовое значение или 0.0 при ошибке.

<br>

### Массив

```c
json_doc_t* query_param_array(httprequest_t* request, const char* param_name, int* ok);
```

Парсит массив из query-строки.

Поддерживаемые форматы:
- `?items[]=a&items[]=b&items[]=c`
- `?items=a,b,c`

**Возвращаемое значение**\
JSON-документ с массивом. Необходимо освободить функцией `json_free()`.

<br>

### Объект

```c
json_doc_t* query_param_object(httprequest_t* request, const char* param_name, int* ok);
```

Парсит объект из query-строки.

Формат: `?filter[name]=John&filter[age]=25`

**Возвращаемое значение**\
JSON-документ с объектом. Необходимо освободить функцией `json_free()`.

## Примеры использования

### Базовые параметры

```c
// URL: /search?query=hello&page=2&limit=10

void search(httpctx_t* ctx) {
    int ok = 0;

    // Строковый параметр
    const char* query = query_param_char(ctx->request, "query", &ok);
    if (!ok || query == NULL) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Parameter 'query' is required");
        return;
    }

    // Числовые параметры с значениями по умолчанию
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

### Параметры с плавающей точкой

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

### Работа с массивами

```c
// URL: /filter?tags[]=php&tags[]=javascript&tags[]=python
// или: /filter?tags=php,javascript,python

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

### Работа с объектами

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

    // Извлекаем значения из объекта
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

### Валидация параметров

```c
void get_product(httpctx_t* ctx) {
    int ok = 0;

    // ID обязателен и должен быть положительным
    int id = query_param_int(ctx->request, "id", &ok);
    if (!ok || id <= 0) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Valid product ID is required");
        return;
    }

    // Получаем продукт...
    char response[64];
    snprintf(response, sizeof(response), "Product ID: %d", id);
    ctx->response->send_data(ctx->response, response);
}
```

### Пагинация

```c
void list_items(httpctx_t* ctx) {
    int ok = 0;

    // Параметры пагинации со значениями по умолчанию
    int page = query_param_int(ctx->request, "page", &ok);
    if (!ok || page < 1) page = 1;

    int per_page = query_param_int(ctx->request, "per_page", &ok);
    if (!ok || per_page < 1 || per_page > 100) per_page = 20;

    int offset = (page - 1) * per_page;

    // Сортировка
    const char* sort = query_param_char(ctx->request, "sort", &ok);
    if (!ok || sort == NULL) sort = "created_at";

    const char* order = query_param_char(ctx->request, "order", &ok);
    if (!ok || order == NULL) order = "desc";

    // Формируем SQL-запрос или используем ORM...
    char response[256];
    snprintf(response, sizeof(response),
        "Page: %d, Per page: %d, Offset: %d, Sort: %s %s",
        page, per_page, offset, sort, order
    );

    ctx->response->send_data(ctx->response, response);
}
```

### Множественные фильтры

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

    // Применяем фильтры...
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

## Обработка ошибок

Всегда проверяйте флаг `ok` после извлечения параметра:

```c
void safe_handler(httpctx_t* ctx) {
    int ok = 0;

    int value = query_param_int(ctx->request, "value", &ok);

    if (!ok) {
        // Параметр отсутствует или имеет неверный формат
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Invalid or missing 'value' parameter");
        return;
    }

    // Параметр успешно получен
    // ...
}
```
