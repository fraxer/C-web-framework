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

## Глобальные middleware

Глобальные middleware настраиваются в `config.json` и применяются ко всем маршрутам сервера:

```json
{
    "servers": {
        "s1": {
            "http": {
                "middlewares": [
                    "middleware_http_test_header",
                    "middleware_http_auth"
                ],
                "routes": { ... }
            }
        }
    }
}
```

### Регистрация глобальных middleware

Глобальные middleware регистрируются в файле `middlewarelist.c`:

```c
// app/middlewares/middlewarelist.c
#include "middleware_registry.h"
#include "httpmiddlewares.h"

middleware_global_fn_t middlewarelist[] = {
    { "middleware_http_test_header", middleware_http_test_header },
    { "middleware_http_auth", middleware_http_auth },
    { "", NULL }  // Конец списка
};
```

## Создание middleware

### Простой middleware

```c
// app/middlewares/httpmiddlewares.h
#ifndef __HTTPMIDDLEWARES__
#define __HTTPMIDDLEWARES__

#include "http.h"

int middleware_http_test_header(httpctx_t* ctx);
int middleware_http_auth(httpctx_t* ctx);

#endif
```

```c
// app/middlewares/httpmiddlewares.c
#include "httpmiddlewares.h"

int middleware_http_test_header(httpctx_t* ctx) {
    // Добавляем заголовок ко всем ответам
    ctx->response->add_header(ctx->response, "X-Powered-By", "C Web Framework");
    return 1;  // Продолжаем выполнение
}
```

### Middleware авторизации

```c
#include "httpmiddlewares.h"
#include "session.h"

int middleware_http_auth(httpctx_t* ctx) {
    const char* session_id = ctx->request->get_cookie(ctx->request, "session_id");
    if (session_id == NULL) {
        ctx->response->status_code = 401;
        ctx->response->send_data(ctx->response, "Unauthorized");
        return 0;  // Прерываем выполнение
    }

    char* session_data = session_get(session_id);
    if (session_data == NULL) {
        ctx->response->status_code = 401;
        ctx->response->send_data(ctx->response, "Session expired");
        return 0;
    }

    // Парсим данные сессии и устанавливаем пользователя
    json_doc_t* doc = json_parse(session_data);
    free(session_data);

    if (doc == NULL) {
        ctx->response->status_code = 401;
        ctx->response->send_data(ctx->response, "Invalid session");
        return 0;
    }

    json_token_t* root = json_root(doc);
    json_token_t* user_id_token = json_object_get(root, "user_id");

    int ok = 0;
    int user_id = json_int(user_id_token, &ok);
    json_free(doc);

    if (!ok || user_id < 1) {
        ctx->response->status_code = 401;
        ctx->response->send_data(ctx->response, "Invalid user");
        return 0;
    }

    // Загружаем пользователя и устанавливаем в контекст
    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, user_id));
    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->status_code = 401;
        ctx->response->send_data(ctx->response, "User not found");
        return 0;
    }

    httpctx_set_user(ctx, user);
    return 1;  // Продолжаем выполнение
}
```

### Middleware с ответом 403

```c
int middleware_http_forbidden(httpctx_t* ctx) {
    ctx->response->send_default(ctx->response, 403);
    return 0;
}
```

## Локальные middleware (в обработчике)

Для применения middleware к конкретному маршруту используйте макрос `middleware()`:

```c
#include "middleware.h"

void secret_page(httpctx_t* ctx) {
    // Проверяем авторизацию перед выполнением
    middleware(
        middleware_http_auth(ctx),
        middleware_check_role(ctx, "admin")
    );

    // Код выполнится только если все middleware вернули 1
    ctx->response->send_data(ctx->response, "Secret content");
}
```

Макрос `middleware()` принимает произвольное количество функций. Если любая из них вернёт 0, выполнение обработчика прервётся.

### Пример проверки роли

```c
int middleware_check_role(httpctx_t* ctx, const char* required_role) {
    user_t* user = httpctx_get_user(ctx);
    if (user == NULL) {
        ctx->response->send_default(ctx->response, 401);
        return 0;
    }

    // Проверяем роль пользователя
    if (!user_has_role(user, required_role)) {
        ctx->response->send_default(ctx->response, 403);
        return 0;
    }

    return 1;
}
```

## Middleware для валидации параметров

```c
int middleware_http_query_param_required(httpctx_t* ctx, char** keys, int size) {
    char message[256] = {0};
    int ok = 0;

    for (int i = 0; i < size; i++) {
        const char* param = query_param_char(ctx->request, keys[i], &ok);
        if (!ok || param == NULL || param[0] == 0) {
            snprintf(message, sizeof(message), "Parameter \"%s\" is required", keys[i]);
            ctx->response->status_code = 400;
            ctx->response->send_data(ctx->response, message);
            return 0;
        }
    }

    return 1;
}

// Использование
void search(httpctx_t* ctx) {
    char* required[] = {"query", "page"};
    middleware(
        middleware_http_query_param_required(ctx, required, 2)
    );

    // Параметры гарантированно присутствуют
    int ok;
    const char* query = query_param_char(ctx->request, "query", &ok);
    int page = query_param_int(ctx->request, "page", &ok);

    // ...
}
```

## Rate Limiting через middleware

Rate limit настраивается в конфигурации и применяется автоматически:

```json
{
    "servers": {
        "s1": {
            "ratelimits": {
                "strict": { "burst": 5, "rate": 1 },
                "normal": { "burst": 100, "rate": 50 }
            },
            "http": {
                "ratelimit": "normal",
                "routes": {
                    "/api/login": {
                        "POST": {
                            "file": "...",
                            "function": "login",
                            "ratelimit": "strict"
                        }
                    }
                }
            }
        }
    }
}
```

## WebSocket middleware

Для WebSocket соединений создаются отдельные middleware:

```c
// app/middlewares/wsmiddlewares.h
#ifndef __WSMIDDLEWARES__
#define __WSMIDDLEWARES__

#include "websockets.h"

int middleware_ws_auth(wsctx_t* ctx);

#endif
```

```c
// app/middlewares/wsmiddlewares.c
#include "wsmiddlewares.h"
#include "session.h"

int middleware_ws_auth(wsctx_t* ctx) {
    // Проверка авторизации для WebSocket
    // ...
    return 1;
}
```
