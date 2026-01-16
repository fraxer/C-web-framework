---
outline: deep
description: Система маршрутизации C Web Framework. Настройка HTTP и WebSocket маршрутов, динамические параметры, middleware и rate limiting.
---

# Маршрутизация

Маршрутизация определяет соответствие между URL-адресами и обработчиками. Поддерживаются HTTP и WebSocket протоколы.

## Базовая конфигурация

```json
"http": {
    "routes": {
        "/": {
            "GET": { "file": "handlers/libindex.so", "function": "index" }
        },
        "/api/users": {
            "GET": { "file": "handlers/libapi.so", "function": "get_users" },
            "POST": { "file": "handlers/libapi.so", "function": "create_user" }
        }
    }
}
```

## Динамические параметры

Параметры маршрута задаются в формате `{name|pattern}`:

```json
"routes": {
    "/api/users/{id|\\d+}": {
        "GET": { "file": "handlers/libapi.so", "function": "get_user" },
        "PUT": { "file": "handlers/libapi.so", "function": "update_user" },
        "DELETE": { "file": "handlers/libapi.so", "function": "delete_user" }
    },
    "/api/posts/{slug|[a-z0-9-]+}": {
        "GET": { "file": "handlers/libapi.so", "function": "get_post" }
    }
}
```

### Извлечение параметров в обработчике

```c
#include "query.h"

void get_user(httpctx_t* ctx) {
    int ok = 0;
    const char* id = query_param_char(ctx->request, "id", &ok);
    // id = "123" для запроса /api/users/123

    if (!ok) {
        // обработка ошибки
        return;
    }

    // Преобразование в число
    int user_id = atoi(id);

    // Поиск пользователя...
}

void get_post(httpctx_t* ctx) {
    int ok = 0;
    const char* slug = query_param_char(ctx->request, "slug", &ok);
    // slug = "my-first-post" для /api/posts/my-first-post
}
```

## Регулярные выражения

Для сложных шаблонов используйте полноценные регулярные выражения:

```json
"routes": {
    "^/managers/{name|[a-z]+}$": {
        "GET": { "file": "handlers/libapi.so", "function": "get_manager" }
    },
    "^/files/(.+)\\.pdf$": {
        "GET": { "file": "handlers/libfiles.so", "function": "get_pdf" }
    }
}
```

## HTTP-методы

Поддерживаемые методы:

| Метод | Описание |
|-------|----------|
| `GET` | Получение ресурса |
| `POST` | Создание ресурса |
| `PUT` | Полное обновление |
| `PATCH` | Частичное обновление |
| `DELETE` | Удаление ресурса |
| `OPTIONS` | Preflight-запросы CORS |

```json
"/api/resource": {
    "GET": { "file": "handlers/lib.so", "function": "get" },
    "POST": { "file": "handlers/lib.so", "function": "create" },
    "PUT": { "file": "handlers/lib.so", "function": "replace" },
    "PATCH": { "file": "handlers/lib.so", "function": "update" },
    "DELETE": { "file": "handlers/lib.so", "function": "delete" },
    "OPTIONS": { "file": "handlers/lib.so", "function": "options" }
}
```

## Статические файлы

Для маршрутов, которые должны отдавать статические файлы без обработки, используйте параметр `static_file` вместо `file` и `function`:

```json
"routes": {
    "/": {
        "GET": { "static_file": "public/index.html" }
    },
    "/favicon.ico": {
        "GET": { "static_file": "public/favicon.ico" }
    },
    "/robots.txt": {
        "GET": { "static_file": "public/robots.txt" }
    }
}
```

Путь к файлу указывается относительно директории `root` сервера.

### Комбинирование с обработчиками

Статические файлы и обработчики можно комбинировать в одном маршруте для разных методов:

```json
"/api/docs": {
    "GET": { "static_file": "public/api-docs.html" },
    "POST": { "file": "handlers/libapi.so", "function": "update_docs" }
}
```

### Rate limiting для статических файлов

К статическим файлам можно применять rate limiting:

```json
"/downloads/report.pdf": {
    "GET": {
        "static_file": "files/report.pdf",
        "ratelimit": "downloads"
    }
}
```

::: tip Когда использовать static_file
Используйте `static_file` для файлов, которые не требуют обработки: HTML-страницы, изображения, документы, favicon и т.д. Это эффективнее, чем создавать обработчик для каждого файла.
:::

## Middleware

Middleware выполняются перед обработчиком:

### Глобальные middleware

```json
"http": {
    "middlewares": ["middleware_auth", "middleware_log"],
    "routes": {
        // Все маршруты проходят через middleware_auth и middleware_log
    }
}
```

### Middleware в обработчике

```c
#include "httpmiddlewares.h"

void protected_handler(httpctx_t* ctx) {
    // Проверка аутентификации
    middleware(
        middleware_http_auth(ctx)
    )

    // Этот код выполнится только если пользователь авторизован
    user_t* user = httpctx_get_user(ctx);
    ctx->response->send_data(ctx->response, "Welcome!");
}
```

## Rate Limiting

Ограничение частоты запросов:

```json
"ratelimits": {
    "default": { "burst": 15, "rate": 15 },
    "strict": { "burst": 1, "rate": 0 },
    "api": { "burst": 100, "rate": 100 }
},
"http": {
    "ratelimit": "default",
    "routes": {
        "/api/users": {
            "GET": {
                "file": "handlers/libapi.so",
                "function": "get_users",
                "ratelimit": "api"
            },
            "POST": {
                "file": "handlers/libapi.so",
                "function": "create_user",
                "ratelimit": "strict"
            }
        }
    }
}
```

## Редиректы

```json
"http": {
    "redirects": {
        "/old-page": "/new-page",
        "/blog/(\\d+)": "/posts/{1}",
        "/user/(.*)": "/profile/{1}"
    }
}
```

Группы захвата доступны через `{1}`, `{2}` и т.д.

## WebSocket маршруты

WebSocket использует аналогичный синтаксис:

```json
"websockets": {
    "default": {
        "file": "handlers/libws.so",
        "function": "ws_default"
    },
    "routes": {
        "/ws": {
            "GET": { "file": "handlers/libws.so", "function": "ws_connect" },
            "POST": { "file": "handlers/libws.so", "function": "ws_message" }
        },
        "/ws/chat/{room|\\d+}": {
            "GET": { "file": "handlers/libws.so", "function": "ws_join_room" },
            "POST": { "file": "handlers/libws.so", "function": "ws_send_message" }
        }
    }
}
```

### WebSocket-обработчик

```c
#include "websockets.h"

void ws_connect(wsctx_t* ctx) {
    ctx->response->send_text(ctx->response, "Connected");
}

void ws_message(wsctx_t* ctx) {
    char* message = websocketsrequest_payload(ctx->request->protocol);

    if (message) {
        ctx->response->send_text(ctx->response, message);
        free(message);
    }
}
```

## Примеры конфигурации

### REST API

```json
"http": {
    "middlewares": ["middleware_cors"],
    "routes": {
        "/api/v1/users": {
            "GET": { "file": "handlers/libusers.so", "function": "list" },
            "POST": { "file": "handlers/libusers.so", "function": "create" }
        },
        "/api/v1/users/{id|\\d+}": {
            "GET": { "file": "handlers/libusers.so", "function": "show" },
            "PUT": { "file": "handlers/libusers.so", "function": "update" },
            "DELETE": { "file": "handlers/libusers.so", "function": "delete" }
        },
        "/api/v1/auth/login": {
            "POST": { "file": "handlers/libauth.so", "function": "login" }
        },
        "/api/v1/auth/logout": {
            "POST": { "file": "handlers/libauth.so", "function": "logout" }
        }
    }
}
```

### Смешанный сайт (страницы + API)

```json
"http": {
    "routes": {
        "/": {
            "GET": { "file": "handlers/libpages.so", "function": "home" }
        },
        "/about": {
            "GET": { "file": "handlers/libpages.so", "function": "about" }
        },
        "/api/data": {
            "GET": { "file": "handlers/libapi.so", "function": "get_data" }
        }
    }
}
```
