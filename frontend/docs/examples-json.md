---
outline: deep
description: Примеры работы с форматом json
---

# Примеры работы с JSON

Готовые примеры парсинга, создания и сериализации JSON-документов.
Полное описание функций — в разделе [JSON](/json).

## Парсинг строки

`json_parse` принимает строку и возвращает готовый документ. Корневой токен
доступен через `json_root`, а тип проверяется функциями `json_is_*`:

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

## Создание документа с нуля

Удобная точка входа — `json_root_create_object` (или `json_root_create_array`),
которая создаёт документ и сразу назначает корневой токен. Токены создаются без
документа: `json_create_string`, `json_create_number`, `json_create_bool`,
`json_create_null`, `json_create_object`, `json_create_array`:

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

Можно начать и с пустого документа, а корень назначить отдельно через
`json_set_root`:

```c
json_doc_t* doc = json_create_empty();
json_token_t* object = json_create_object();
json_set_root(doc, object);
```

## Вложенные объекты и массивы

Токены-контейнеры можно свободно вкладывать друг в друга. Значения
создаются заранее и передаются в `json_object_set` / `json_array_append`:

```c
#include "http.h"
#include "json.h"

void nested_json(httpctx_t* ctx) {
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);

    // Вложенный объект "address"
    json_token_t* address = json_create_object();
    json_object_set(address, "city", json_create_string("Berlin"));
    json_object_set(address, "zip", json_create_number(10115));
    json_object_set(root, "address", address);

    // Массив "tags" с prepend/append
    json_token_t* tags = json_create_array();
    json_object_set(root, "tags", tags);

    json_array_append(tags, json_create_string("php"));
    json_array_append(tags, json_create_string("c"));
    json_array_prepend(tags, json_create_string("first"));
    json_array_append_to(tags, 1, json_create_string("sql"));  // вставка по индексу

    // Изменение значения по индексу
    json_token_t* second = json_array_get(tags, 1);
    if (second) {
        json_token_set_string(second, "rust");
    }

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(doc));

    json_free(doc);
}
```

## Чтение типизированных значений

Числовые геттеры `json_int`, `json_uint`, `json_llong`, `json_double` принимают
выходной параметр `int* ok` — он равен нулю, если токен не является числом.
Проверить тип помогает `json_is_number` (и другие `json_is_*`):

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

Строки и логические значения читаются напрямую — без параметра `ok`:
`json_string(token)`, `json_bool(token)`. Длину строки в байтах возвращает
`json_string_size`.

## Обход объекта и массива

Единый итератор `json_it_t` работает и для объектов, и для массивов. Для объекта
ключ доступен через `json_it_key`, значение — через `json_it_value`:

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

При обходе массива `json_it_key` возвращает `NULL` — есть только значения.
Удалить текущий элемент на ходу можно функцией `json_it_erase`.

## Сериализация и ответ

`json_stringify` возвращает строку, привязанную к документу, и освобождается
вместе с ним через `json_free`. Флаг `ascii_mode = 1` кодирует все символы вне
ASCII как `\uXXXX` — удобно для логов и совместимости со старыми клиентами:

```c
#include "http.h"
#include "json.h"

void stringify_json(httpctx_t* ctx) {
    json_doc_t* doc = json_parse("[\"Привет 😅\", 42, true]");
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

Если строку нужно пережить документ — используйте `json_stringify_detach`. Она
возвращает отдельную копию, которую необходимо освободить вручную через `free`:

```c
char* body = json_stringify_detach(doc);
if (body) {
    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, body);
    free(body);
}
json_free(doc);
```
