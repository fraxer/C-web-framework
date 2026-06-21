---
outline: deep
description: Парсинг query-параметров в C Web Framework. Извлечение строк, чисел, массивов и объектов из URL через типизированный API.
---

# Query-параметры

Query-параметры (часть URL после `?`) — самый частый способ передать в обработчик небольшие данные: фильтры, номер страницы, идентификаторы, сортировку. Фреймворк парсит их один раз при разборе запроса и предоставляет типизированный API для безопасного извлечения значений.

Query-параметры доступны в обработчике как связный список `query_t*` через поле `ctx->request->query_`. Все функции извлечения принимают его первым аргументом, имя параметра — вторым, а флаг успеха `int* ok` — третьим.

## Структура query_t

Каждый параметр представлен узлом связного списка (определён в `query.h`):

```c
typedef struct url_query {
    const char* key;     // имя параметра
    const char* value;   // значение (URL-декодированное)
    struct url_query* next;
} query_t;
```

| Поле | Описание |
|------|----------|
| `key` | Имя параметра. Уникальность не гарантируется — при дублировании берётся первое совпадение. |
| `value` | Значение параметра. **URL-декодируется** парсером автоматически (`%20` → пробел, `+` → пробел). |
| `next` | Указатель на следующий узел (`NULL` у последнего). |

::: tip Поведение парсера
- Формат строки: `key1=value1&key2=value2&key3`.
- Параметр без значения (`?flag`) сохраняется как `{key:"flag", value:""}`.
- Всё, что идёт после `#`, считается фрагментом и не разбирается.
- Ключи и значения URL-декодируются.
:::

## Обзор функций

| Функция | Возвращает | Назначение |
|---------|-----------|-----------|
| `query_param_char` | `const char*` | Значение как строка |
| `query_param_int` | `int` | Знаковое целое |
| `query_param_uint` | `unsigned int` | Беззнаковое целое |
| `query_param_long` | `long` | Знаковое `long` |
| `query_param_ulong` | `unsigned long` | Беззнаковое `long` |
| `query_param_float` | `float` | Одинарная точность |
| `query_param_double` | `double` | Двойная точность |
| `query_param_ldouble` | `long double` | Расширенная точность |
| `query_param_array` | `json_doc_t*` | JSON-массив из значения параметра |
| `query_param_object` | `json_doc_t*` | JSON-объект из значения параметра |

Все функции принимают `(query_t* query, const char* param_name, int* ok)`. Флаг `ok` устанавливается в `1` при успехе, в `0` — если параметр отсутствует или значение не удалось разобрать. Перед вызовом флаг принято инициализировать нулём.

## Строковый параметр

```c
#include "query.h"

const char* query_param_char(query_t* query, const char* param_name, int* ok);
```

Возвращает значение параметра как строку. Если параметр с таким ключом есть в запросе — `ok` равен `1`, даже когда значение пустое (`?flag` → возвращается `""`, `ok = 1`). Если ключа нет — возвращается `NULL`, `ok = 0`.

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

::: tip Различайте «нет параметра» и «пустое значение»
`ok = 1` говорит лишь о том, что ключ присутствует. Чтобы отличить `?q` (пустое значение) от отсутствующего параметра, дополнительно проверяйте `value == NULL` или `value[0] == 0`, если пустое значение недопустимо.
:::

## Целочисленные параметры

```c
int          query_param_int(query_t* query, const char* param_name, int* ok);
unsigned int query_param_uint(query_t* query, const char* param_name, int* ok);
long         query_param_long(query_t* query, const char* param_name, int* ok);
unsigned long query_param_ulong(query_t* query, const char* param_name, int* ok);
```

Возвращает число либо `0` при ошибке. `ok = 0`, если параметр отсутствует или значение не является корректным целым (пустая строка, буквы, знак у беззнакового типа и т. п.). Всегда проверяйте `ok`, прежде чем полагаться на возвращённое значение — `0` может быть и валидным.

```c
int ok = 0;
int page = query_param_int(ctx->request->query_, "page", &ok);
if (!ok) page = 1;  // значение по умолчанию
```

## Параметры с плавающей точкой

```c
float       query_param_float(query_t* query, const char* param_name, int* ok);
double      query_param_double(query_t* query, const char* param_name, int* ok);
long double query_param_ldouble(query_t* query, const char* param_name, int* ok);
```

Возвращает число либо `0.0` при ошибке. `ok = 0`, если параметр отсутствует или значение не удалось преобразовать функциями `strtof` / `strtod` / `strtold`.

```c
int ok = 0;
double price = query_param_double(ctx->request->query_, "price", &ok);
if (!ok) {
    ctx->response->send_data(ctx->response, "Invalid price");
    return;
}
```

## Массив

```c
json_doc_t* query_param_array(query_t* query, const char* param_name, int* ok);
```

Разбирает **значение** параметра как JSON-массив. Парсер URL-декодирует значение, затем передаёт его в `json_parse()` и проверяет, что корень — массив.

Формат значения — корректный JSON-массив:

```
?tags=["php","javascript","python"]
```

Так как значение содержит спецсимволы, на клиенте его нужно URL-кодировать:

```
?tags=%5B%22php%22%2C%22javascript%22%2C%22python%22%5D
```

**Возвращаемое значение** — указатель на `json_doc_t*` с массивом либо `NULL`. Документ необходимо освободить функцией `json_free()`. `ok = 0`, если параметр отсутствует, значение пустое или не является JSON-массивом.

::: warning Поддерживаются только JSON-значения
PHP-синтаксис `?tags[]=a&tags[]=b` и формат через запятую `?tags=a,b,c` **не** поддерживаются — значение должно быть валидным JSON-массивом. Для перебора элементов используйте `json_array_size()` и `json_array_get()`.
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

## Объект

```c
json_doc_t* query_param_object(query_t* query, const char* param_name, int* ok);
```

Разбирает **значение** параметра как JSON-объект. Работает аналогично `query_param_array`, но корень должен быть объектом.

Формат значения — корректный JSON-объект:

```
?filter={"status":"active","role":"admin"}
```

URL-кодированный вид:

```
?filter=%7B%22status%22%3A%22active%22%2C%22role%22%3A%22admin%22%7D
```

**Возвращаемое значение** — указатель на `json_doc_t*` с объектом либо `NULL`. Документ необходимо освободить функцией `json_free()`. `ok = 0`, если параметр отсутствует, значение пустое или не является JSON-объектом.

::: warning Поддерживаются только JSON-значения
Скобочный синтаксис `?filter[name]=John&filter[age]=25` **не** поддерживается — значение должно быть валидным JSON-объектом. Для доступа к полям используйте `json_object_get()`.
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

    json_token_t* status = json_object_get(filter, "status");
    json_token_t* role   = json_object_get(filter, "role");
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

## Примеры использования

### Пагинация и сортировка

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

### Несколько фильтров

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

### Обязательный числовой идентификатор

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

## Проверка наличия через middleware

Для проверки обязательных параметров в кодовой базе есть готовый middleware `middleware_http_query_param_required` — он проверяет, что все перечисленные параметры присутствуют и непусты:

```c
int middleware_http_query_param_required(httpctx_t* ctx, char** keys, int size);
```

Возвращает `1` (пропустить), если каждый ключ найден и его значение непустое; иначе отправляет ответ с описанием отсутствующего параметра и возвращает `0` (отклонить). Удобно вызывать через макрос `middleware()` вместе с `args_str()`:

```c
#include "httpmiddlewares.h"

void userviewget(httpctx_t* ctx) {
    // Прерывает обработчик, если параметр "id" отсутствует или пуст
    middleware(
        middleware_http_query_param_required(ctx, args_str("id"))
    )

    int param_ok = 0;
    const int user_id = query_param_int(ctx->request->query_, "id", &param_ok);
    if (!param_ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    // Дальнейшая обработка...
}
```

`args_str(...)` разворачивается в пару `(char*[]){...}, count`, так что список ключей задаётся прямо в вызове. Макрос `middleware(...)` выполняет проверку и досрочно выходит из обработчика (`return`), если она не пройдена.

## Обработка ошибок

Правило простое: всегда проверяйте флаг `ok` сразу после извлечения параметра. Для чисел это обязательно — `0` может быть как валидным значением, так и признаком ошибки:

```c
void safe_handler(httpctx_t* ctx) {
    int ok = 0;

    int value = query_param_int(ctx->request->query_, "value", &ok);
    if (!ok) {
        // Параметр отсутствует или имеет неверный формат
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Invalid or missing 'value' parameter");
        return;
    }

    // Параметр успешно получен — value корректен
}
```

::: tip Память
- `query_param_char` возвращает указатель, принадлежащий запросу, — освобождать его не нужно.
- `query_param_array` и `query_param_object` возвращают свежий `json_doc_t*` — обязательно освобождайте его функцией `json_free()`.
:::
