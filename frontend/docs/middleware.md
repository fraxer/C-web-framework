---
outline: deep
description: Middleware в C Web Framework. Создание цепочек обработчиков, проверка авторизации, валидация запросов.
---

# Middleware

Middleware — промежуточные обработчики, которые выполняются перед основным обработчиком маршрута. Используются для аутентификации, валидации, логирования и других задач.

## Принцип работы

Middleware — это функция, которая принимает контекст запроса и возвращает:

- `1` — продолжить выполнение цепочки
- `0` — прервать выполнение (основной обработчик не будет вызван)

```c
typedef int (*middleware_fn_p)(void*);
```

При возврате `0` middleware должен сам сформировать ответ (статус, тело) — например, через `ctx->response->send_data()` или `send_default()`. Возврат `1` просто передаёт управление дальше.

## Глобальные middleware

Глобальные middleware настраиваются в `config.json` и применяются ко всем маршрутам HTTP-сервера. Имена в массиве ссылаются на middleware, зарегистрированные в реестре (см. ниже):

```json
{
    "servers": {
        "s1": {
            "http": {
                "middlewares": [
                    "middleware_http_test_header"
                ],
                "routes": { ... }
            }
        }
    }
}
```

### Регистрация в реестре

Глобальные middleware регистрируются в файле `app/middlewares/middlewarelist.c` внутри функции `middlewares_init()` через вызов `middleware_registry_register(name, fn)`. Эта функция вызывается во время инициализации приложения (из `moduleloader`):

```c
// app/middlewares/middlewarelist.c
#include "middleware_registry.h"
#include "httpmiddlewares.h"
#include "wsmiddlewares.h"
#include "log.h"

int middlewares_init(void) {
    if (!middleware_registry_register("middleware_http_forbidden", (middleware_fn_p)middleware_http_forbidden)) {
        log_error("middlewares_init: failed to register middleware_http_forbidden\n");
        return 0;
    }

    if (!middleware_registry_register("middleware_http_test_header", (middleware_fn_p)middleware_http_test_header)) {
        log_error("middlewares_init: failed to register middleware_http_test_header\n");
        return 0;
    }

    // Добавляйте новые middleware здесь:
    // middleware_registry_register("middleware_cors", (middleware_fn_p)middleware_cors);

    return 1;
}
```

::: tip Имя = ключ в config.json
Строка, переданная первым аргументом в `middleware_registry_register()`, — это то самое имя, которое указывается в массиве `middlewares` конфигурации.
:::

### API реестра

Интерфейс реестра объявлен в `core/framework/middleware/middleware_registry.h`:

| Функция | Назначение |
| --- | --- |
| `middlewares_init()` | Регистрирует все middleware приложения при старте. |
| `middleware_registry_register(name, handler)` | Регистрирует middleware по имени. `1` — успех, `0` — ошибка (реестр переполнен / дубликат). |
| `middleware_by_name(name)` | Возвращает функцию middleware по имени или `NULL`. |
| `middleware_registry_get_all(&count)` | Возвращает массив всех зарегистрированных middleware. |
| `middleware_registry_clear()` | Очищает реестр (используется при перезагрузке конфигурации). |

## Создание HTTP middleware

HTTP middleware принимают `httpctx_t*`. Заголовочный файл подключает `httpctx.h` и `middleware.h`:

```c
// app/middlewares/httpmiddlewares.h
#ifndef __HTTPMIDDLEWARES__
#define __HTTPMIDDLEWARES__

#include "httpctx.h"
#include "middleware.h"

int middleware_http_forbidden(httpctx_t* ctx);
int middleware_http_test_header(httpctx_t* ctx);
int middleware_http_query_param_required(httpctx_t* ctx, char** keys, int size);
int middleware_http_auth(httpctx_t* ctx);

#endif
```

### Простой middleware

```c
// app/middlewares/httpmiddlewares.c
#include "httpmiddlewares.h"

int middleware_http_test_header(httpctx_t* ctx) {
    ctx->response->add_header(ctx->response, "X-Test-Header", "test");
    return 1;  // Продолжаем выполнение
}
```

### Запрет доступа (403)

```c
int middleware_http_forbidden(httpctx_t* ctx) {
    ctx->response->send_default(ctx->response, 403);
    return 0;  // Прерываем выполнение
}
```

### Middleware авторизации

Загружает пользователя из сессии и делает его доступным в контексте через `httpctx_set_user()`:

```c
#include "httpmiddlewares.h"
#include "session.h"

int middleware_http_auth(httpctx_t* ctx) {
    const char* session_id = ctx->request->get_cookie(ctx->request, "session_id");
    if (session_id == NULL) {
        ctx->response->send_data(ctx->response, "Session id not found");
        return 0;
    }

    int result = 0;
    json_doc_t* document = NULL;

    // Первый аргумент — имя сессии из config.json (секция "sessions")
    char* session_data = session_get("backend", session_id);
    if (session_data == NULL) {
        ctx->response->send_data(ctx->response, "Session not found");
        return 0;
    }

    document = json_parse(session_data);
    if (document == NULL) {
        ctx->response->send_data(ctx->response, "");
        goto failed;
    }

    json_token_t* object = json_root(document);
    if (!json_is_object(object)) {
        ctx->response->send_data(ctx->response, "Session data is not object");
        goto failed;
    }

    json_token_t* token_user_id = json_object_get(object, "user_id");
    if (!json_is_number(token_user_id)) {
        ctx->response->send_data(ctx->response, "Session data is not valid");
        goto failed;
    }

    int ok = 0;
    const int user_id = json_int(token_user_id, &ok);
    if (!ok || user_id < 1) {
        ctx->response->send_data(ctx->response, "User is not valid");
        goto failed;
    }

    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, user_id));
    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->send_data(ctx->response, "User not found");
        goto failed;
    }

    httpctx_set_user(ctx, user);
    result = 1;

failed:
    free(session_data);
    json_free(document);
    return result;
}
```

Получить пользователя в обработчике можно макросом `httpctx_get_user(ctx)` — он приводится к `user_t*`:

```c
user_t* user = httpctx_get_user(ctx);
```

## Локальные middleware (в обработчике)

Для применения middleware к конкретному маршруту используется макрос `middleware()`. Он принимает произвольное число вызовов и раскрывается в короткое замыкание:

```c
#define middleware(...) if (!(MW_ITEM(MW_VA_NARGS(__VA_ARGS__), __VA_ARGS__))) return;
// раскрытие для двух аргументов:
// if (!(fn1(ctx) && fn2(ctx))) return;
```

Если хотя бы один middleware вернёт `0`, обработчик немедленно завершается через `return`. Поэтому макрос применим **только внутри функций, возвращающих `void`** (обработчиков маршрутов).

```c
// app/routes/auth/auth.c
void secret_page(httpctx_t* ctx) {
    // Код ниже выполнится только если middleware вернул 1
    middleware(
        middleware_http_auth(ctx)
    );

    ctx->response->send_data(ctx->response, "done");
}
```

Несколько middleware перечисляются через запятую и выполняются слева направо до первого `0`:

```c
void admin_page(httpctx_t* ctx) {
    middleware(
        middleware_http_auth(ctx),
        middleware_check_role(ctx, "admin")
    );

    ctx->response->send_data(ctx->response, "admin area");
}
```

### Свои middleware для обработчика

Такие middleware не нужно регистрировать в реестре — достаточно объявить их и вызывать через макрос. Пример проверки роли пользователя:

```c
int middleware_check_role(httpctx_t* ctx, const char* required_role) {
    user_t* user = httpctx_get_user(ctx);
    if (user == NULL) {
        ctx->response->send_default(ctx->response, 401);
        return 0;
    }

    if (!user_has_role(user, required_role)) {  // ваша проверка роли
        ctx->response->send_default(ctx->response, 403);
        return 0;
    }

    return 1;
}
```

## Валидация параметров

`middleware_http_query_param_required()` проверяет наличие обязательных query-параметров. Для передачи списка ключей удобно использовать макрос `args_str(...)` из `core/misc/macros.h` — он строит массив строк прямо из аргументов:

```c
int middleware_http_query_param_required(httpctx_t* ctx, char** keys, int size) {
    char message[256] = {0};
    int ok = 0;

    for (int i = 0; i < size; i++) {
        const char* param = query_param_char(ctx->request->query_, keys[i], &ok);
        if (!ok) {
            return 0;
        }

        if (param == NULL || param[0] == 0) {
            sprintf(message, "param \"%s\" not found", keys[i]);
            ctx->response->send_data(ctx->response, message);
            return 0;
        }
    }

    return 1;
}
```

Использование в обработчике:

```c
// app/routes/middleware/middleware.c
void example(httpctx_t* ctx) {
    middleware(
        middleware_http_query_param_required(ctx, args_str("a", "abc"))
    );

    // Параметры гарантированно присутствуют
    ctx->response->send_data(ctx->response, "done");
}
```

Эквивалентная запись без макроса — обычный `if`:

```c
if (!middleware_http_query_param_required(ctx, args_str("a", "abc"))) return;
```

## WebSocket middleware

Для WebSocket-соединений создаются отдельные middleware с сигнатурой `int name(wsctx_t* ctx, ...)`. Заголовочный файл подключает `wsctx.h` и `middleware.h`:

```c
// app/middlewares/wsmiddlewares.h
#ifndef __WSMIDDLEWARES__
#define __WSMIDDLEWARES__

#include "wsctx.h"
#include "middleware.h"

int middleware_ws_query_param_required(wsctx_t* ctx, char** keys, int size);

#endif
```

Доступ к query-параметрам рукопожатия идёт через ресурс протокола:

```c
// app/middlewares/wsmiddlewares.c
#include "websockets.h"
#include "wsmiddlewares.h"

int middleware_ws_query_param_required(wsctx_t* ctx, char** keys, int size) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;
    char message[256] = {0};

    for (int i = 0; i < size; i++) {
        int ok = 0;
        const char* param = protocol->get_query(protocol, keys[i], &ok);
        if (!ok || param == NULL || param[0] == 0) {
            sprintf(message, "param <%s> not found", keys[i]);
            ctx->response->send_data(ctx, message);
            return 0;
        }
    }

    return 1;
}
```

::: warning WebSocket middleware — только локальные
WS-middleware не регистрируются глобально в `middlewares_init()`. Они применяются локально — через макрос `middleware()` в самом обработчике WebSocket:

```c
// app/routes/ws/wsindex.c
void middleware_test(wsctx_t* ctx) {
    middleware(
        middleware_ws_query_param_required(ctx, args_str("abc"))
    );

    ctx->response->send_data(ctx, "done");
}
```
:::

## Rate Limiting

Ограничение частоты запросов настраивается отдельно — в `config.json`, а не через систему middleware. Именованные бакеты объявляются в `servers.<id>.ratelimits`, а привязка идёт на уровне HTTP-сервера (`ratelimit`) или конкретного маршрута:

```json
{
    "servers": {
        "s1": {
            "ratelimits": {
                "one":  { "burst": 1,  "rate": 0 },
                "two":  { "burst": 15, "rate": 15 }
            },
            "http": {
                "ratelimit": "one",
                "routes": {
                    "/api/login": {
                        "POST": {
                            "file": "...",
                            "function": "login",
                            "ratelimit": "two"
                        }
                    }
                }
            }
        }
    }
}
```

- `burst` — размер корзины (максимальное число моментальных запросов);
- `rate` — скорость пополнения корзины (запросов в секунду); `0` — пополнения нет.
