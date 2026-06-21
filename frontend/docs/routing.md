---
outline: deep
description: Система маршрутизации C Web Framework. Настройка HTTP и WebSocket маршрутов, динамические параметры, middleware, rate limiting и редиректы.
---

# Маршрутизация

Маршрутизация определяет соответствие между URL-адресами и обработчиками. Поддерживаются протоколы HTTP и WebSocket. Маршруты описываются в секции `servers.<id>` файла `config.json` — внутри объектов `http` и `websockets`.

## Базовая конфигурация

Ключ объекта `routes` — путь, значение — объект «метод → обработчик»:

```json
"http": {
    "routes": {
        "/": {
            "GET": { "file": "handlers/libindex.so", "function": "index" }
        },
        "/api/users": {
            "GET":  { "file": "handlers/libapi.so", "function": "get_users" },
            "POST": { "file": "handlers/libapi.so", "function": "create_user" }
        }
    }
}
```

Поля обработчика:

| Поле          | Тип    | Описание                                                                             |
|---------------|--------|--------------------------------------------------------------------------------------|
| `file`        | строка | Путь до `.so` с обработчиком (относительно корня процесса или абсолютный)            |
| `function`    | строка | Имя функции-обработчика                                                              |
| `static_file` | строка | Путь к статическому файлу; если задано, отдаётся файл вместо вызова `file`/`function` |
| `ratelimit`   | строка | Имя профиля rate limiting для конкретного маршрута (переопределяет `http.ratelimit`) |

## Сопоставление маршрутов

Маршруты проверяются **в порядке объявления** — побеждает первое совпадение. Есть два типа маршрутов:

* **Примитивные** — путь без параметров и символов регулярных выражений (`/`, `/api/users`). Сопоставляются быстрым посимвольным сравнением.
* **Параметризованные / регулярные** — путь с параметрами `{...}` или символами регулярок (`*`, `[`, `(`, `+`, `^`, `$`, `|`). Компилируются в регулярное выражение PCRE.

Если для совпавшего маршрута не задан обработчик для текущего метода, проверка продолжается по следующим маршрутам. Если ни один маршрут не подошёл — возвращается `404`.

## Динамические параметры

Параметры маршрута задаются в формате `{name|pattern}`, где `pattern` — регулярное выражение PCRE:

```json
"routes": {
    "/api/users/{id|\\d+}": {
        "GET":    { "file": "handlers/libapi.so", "function": "get_user" },
        "PUT":    { "file": "handlers/libapi.so", "function": "update_user" },
        "DELETE": { "file": "handlers/libapi.so", "function": "delete_user" }
    },
    "/api/posts/{slug|[a-z0-9-]+}": {
        "GET": { "file": "handlers/libapi.so", "function": "get_post" }
    }
}
```

Значения параметров попадают в строку запроса и читаются как query-параметры.

### Извлечение параметров в обработчике

```c
#include "query.h"

void get_user(httpctx_t* ctx) {
    int ok = 0;
    const char* id = query_param_char(ctx->request->query_, "id", &ok);
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
    const char* slug = query_param_char(ctx->request->query_, "slug", &ok);
    // slug = "my-first-post" для /api/posts/my-first-post
}
```

::: warning Именованные параметры и регулярные выражения
Именованные параметры `{name|pattern}` нельзя смешивать с «сырыми» символами регулярных выражений (`* [ ] ( ) + ^ | $`) в одном пути — сам параметр и есть регулярное выражение. Для свободных шаблонов используйте полноценные регулярные выражения (см. ниже).
:::

## Регулярные выражения

Для сложных шаблонов используйте полноценные регулярные выражения. Группы захвата доступны в обработчике как нумерованные параметры:

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

| Метод      | Описание                |
|------------|-------------------------|
| `GET`      | Получение ресурса       |
| `POST`     | Создание ресурса        |
| `PUT`      | Полное обновление       |
| `PATCH`    | Частичное обновление    |
| `DELETE`   | Удаление ресурса        |
| `HEAD`     | Заголовки без тела      |
| `OPTIONS`  | Preflight-запросы CORS  |

```json
"/api/resource": {
    "GET":     { "file": "handlers/lib.so", "function": "get" },
    "POST":    { "file": "handlers/lib.so", "function": "create" },
    "PUT":     { "file": "handlers/lib.so", "function": "replace" },
    "PATCH":   { "file": "handlers/lib.so", "function": "update" },
    "DELETE":  { "file": "handlers/lib.so", "function": "delete" },
    "HEAD":    { "file": "handlers/lib.so", "function": "head" },
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
    "GET":  { "static_file": "public/api-docs.html" },
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

Middleware выполняются перед обработчиком и применяются ко всем маршрутам секции (`http.middlewares` / `websockets.middlewares`):

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

Профили rate limiting определяются на уровне сервера — в секции `ratelimits` (рядом с `http` и `websockets`). Каждый профиль задаёт `burst` (ёмкость корзины / пиковое число запросов) и `rate` (скорость восполнения токенов в секунду):

```json
"s1": {
    "ratelimits": {
        "default": { "burst": 15,  "rate": 15 },
        "strict":  { "burst": 1,   "rate": 0 },
        "api":     { "burst": 100, "rate": 100 }
    },
    "http": {
        "ratelimit": "default",
        "routes": {
            "/api/users": {
                "GET":  { "file": "handlers/libapi.so", "function": "get_users",   "ratelimit": "api" },
                "POST": { "file": "handlers/libapi.so", "function": "create_user", "ratelimit": "strict" }
            }
        }
    }
}
```

* `http.ratelimit` — профиль по умолчанию для всех HTTP-маршрутов сервера.
* `ratelimit` в конкретном маршруте переопределяет профиль для него.

## Редиректы

Редиректы описываются в секции `http.redirects`. Ключ — регулярное выражение PCRE, значение — целевой путь. Группы захвата подставляются через `{1}`, `{2}` и т.д.:

```json
"http": {
    "redirects": {
        "/user": "/persons",
        "/user(.*)/(\\d)": "/user-{1}-{2}",
        "/section1/(\\d+)/section2/(\\d+)/section3": "/one/{1}/two/{2}/three"
    }
}
```

## WebSocket маршруты

WebSocket настраивается в секции `websockets`. Поддерживаются обработчик по умолчанию (`default`), общий профиль `ratelimit`, `middlewares` и `routes`:

```json
"websockets": {
    "default": {
        "file": "handlers/libws.so",
        "function": "ws_default"
    },
    "ratelimit": "default",
    "routes": {
        "/ws": {
            "GET":  { "file": "handlers/libws.so", "function": "ws_connect" },
            "POST": { "file": "handlers/libws.so", "function": "ws_message" }
        },
        "/ws/chat/{room|\\d+}": {
            "GET":  { "file": "handlers/libws.so", "function": "ws_join_room" },
            "POST": { "file": "handlers/libws.so", "function": "ws_send_message" }
        }
    }
}
```

* `default` — обработчик, вызываемый когда ни один маршрут не подошёл. Если секция `websockets` не задана, используется встроенный обработчик по умолчанию.
* `ratelimit` — профиль rate limiting по умолчанию для всех WS-маршрутов.
* `middlewares` — middleware, применяемые ко всем WS-маршрутам.

Поддерживаемые методы для WebSocket-маршрутов: `GET`, `POST`, `PATCH`, `DELETE`.

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
    "ratelimit": "default",
    "routes": {
        "/api/v1/users": {
            "GET":  { "file": "handlers/libusers.so", "function": "list" },
            "POST": { "file": "handlers/libusers.so", "function": "create" }
        },
        "/api/v1/users/{id|\\d+}": {
            "GET":    { "file": "handlers/libusers.so", "function": "show" },
            "PUT":    { "file": "handlers/libusers.so", "function": "update" },
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
