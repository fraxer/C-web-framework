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
| `static_file` | строка | Путь к статическому файлу; если задано, отдаётся файл вместо вызова `file`/`function`. Поддерживает подстановку групп захвата `{1}`, `{2}`, … из шаблона маршрута |
| `storage`     | строка | Имя хранилища из секции [`storages`](/storage), из которого берётся `static_file`, вместо `root` сервера. Только вместе со `static_file` — см. [Отдача из хранилища](#отдача-из-хранилища) |
| `cache_control` | строка | Заголовок `Cache-Control` для того, чем отвечает маршрут — файлом или обработчиком (умолчание для файла: `no-cache`) |
| `ratelimit`   | строка | Имя профиля rate limiting для конкретного маршрута (переопределяет `http.ratelimit`) |

## Сопоставление маршрутов

Маршруты проверяются **в порядке объявления** — побеждает первое совпадение. Есть два типа маршрутов:

* **Примитивные** — путь без параметров и операторов регулярных выражений (`/`, `/api/users`, `/api/v1.0`). Сопоставляются посимвольным сравнением; не совпал — сразу проверяется следующий маршрут, регулярное выражение для такого пути не запускается вовсе.
* **Параметризованные / регулярные** — путь с параметрами `{...}` или операторами регулярок (`*`, `+`, `(`, `)`, `[`, `]`, `|`, `^`, `$`, `\`). Компилируются в регулярное выражение PCRE.

Если для совпавшего маршрута не задан обработчик для текущего метода, проверка продолжается по следующим маршрутам. Если ни один маршрут не подошёл — возвращается `404`.

### Точка и знак вопроса

В **простом** пути — где нет ни параметров `{...}`, ни одного оператора из списка выше — точка и знак вопроса означают сами себя: при разборе они экранируются. Маршрут `/api/v1.0` отвечает только на `/api/v1.0` и с `/api/v1x0` не совпадает.

Как только в пути появляется хотя бы один оператор, вся строка читается как шаблон, и точка снова значит «любой символ» — иначе `/assets/(.*)` перестал бы захватывать имена файлов с расширением. То же и в путях с параметрами: в `/files/{name|[a-z]+}.json` точка совпадает с любым символом, поэтому такому маршруту подойдёт и `/files/abcxjson`. Нужен именно символ точки — экранируйте его сами (в JSON обратный слэш удваивается):

```json
"routes": {
    "/api/v1.0": {
        "GET": { "file": "handlers/libapi.so", "function": "v1_0" }
    },
    "/files/{name|[a-z]+}\\.json": {
        "GET": { "file": "handlers/libfiles.so", "function": "get_json" }
    }
}
```

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

В шаблоне маршрута — не больше 64 групп захвата, считая группы параметров `{name|…}`. Маршрут с бо́льшим числом групп не загружается, а причина пишется в журнал.

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

В пути `static_file` можно сослаться на группы захвата маршрута через `{1}`, `{2}`, … (то же написание, что у `redirects`) — так один маршрут покрывает целый каталог, что удобно для артефактов сборки, у которых в имени хеш содержимого:

```json
"/assets/(.*)": {
    "GET": {
        "static_file": "/assets/{1}",
        "cache_control": "public, max-age=31536000, immutable"
    }
}
```

По умолчанию каждый файловый ответ уходит с `Cache-Control: no-cache`, то есть клиент перепроверяет файл при каждом использовании. `cache_control` переопределяет это для маршрута — вместе с именами файлов, содержащими хеш, браузер перестаёт спрашивать про файл вообще. Файлы, отданные через маршрут без `cache_control`, сохраняют значение по умолчанию.

`cache_control` работает и для маршрутов с обработчиком, где выступает умолчанием маршрута: обработчик, выставивший `Cache-Control` сам, сохраняет своё значение. Отсутствующий `static_file` по-прежнему отвечает 404 без этого заголовка, так что длинный `max-age` никогда не ляжет на ошибку.

### Отдача из хранилища

По умолчанию `static_file` отсчитывается от `root` сервера. Ключ `storage` переносит маршрут в именованное хранилище из секции [`storages`](/storage) — и файловое, и S3. Связка «маршрут → хранилище» становится строкой конфигурации, а не обработчиком, который её программирует:

```json
"/assets/(.*)": {
    "GET": {
        "static_file": "{1}",
        "storage": "assets",
        "cache_control": "public, max-age=31536000, immutable"
    }
},
"/videos/(.*)": {
    "GET": { "static_file": "{1}", "storage": "media" }
}
```

Типичный случай — вёрстка лежит рядом с кодом на быстром диске, а видео хранить там накладно: `assets` указывает на каталог, `media` — на S3-бакет, и клиент об этом не знает.

Маршрут без `storage` работает как раньше, от `root`.

Ключ задаётся на методе, а не на маршруте целиком: у `GET` и `HEAD` одного пути он указывается отдельно. Метод без записи этот маршрут пропускает, и если дальше ничего не совпало, файл ищется в `root`.

#### Чем storage-маршрут отличается от обычной статики

Одинаково для файлового хранилища и для S3:

* **Работают middleware и ratelimit маршрута.** У статики от `root` middleware не выполняются: приватное видео за авторизацией настраивается, а не пишется обработчиком, а обращение в S3 стоит трафика, который надо уметь ограничить. Сначала проверяется ratelimit (отказ — `429` с `Retry-After`), затем middleware; запрос, отклонённый middleware, получает её ответ, и в хранилище за ним не ходят.
* **`cache_control` маршрута ложится только на то, что хранилище отдало** — `200`, `206` и `304`. Ни `404`, ни `429`, ни отказ middleware, ни ошибка S3 этот заголовок не получают, так что годовой `max-age` не закеширует ошибку.
* **Ответ готовится в рабочем потоке** из пула `main.threads`, как у обработчиков, а не в цикле событий: иначе middleware негде было бы выполнить.

```json
"/private/(.*)": {
    "GET": {
        "static_file": "{1}",
        "storage": "media",
        "ratelimit": "downloads",
        "cache_control": "private, max-age=3600"
    }
}
```

Middleware берутся из общего списка `http.middlewares` вхоста — отдельного списка у маршрута нет. Если на других маршрутах вхоста та же проверка не нужна, middleware решает сама, по пути запроса.

#### Файловое хранилище

Отдаёт файл так же, как статика от `root`: `sendfile`, диапазонные запросы, `ETag`, `gzip_static`. Путь приходит из запроса, поэтому шаблоны (`*`, `?`, `[`, `~`) в нём отклоняются, а не раскрываются, и выход за пределы хранилища (`..`) тоже. Каталог, FIFO и прочие нерегулярные файлы отвечают `404`.

#### S3-хранилище

Объект сначала запрашивается методом `HEAD` — оттуда берутся размер, `ETag`, `Last-Modified` и `Content-Type`; HTTP-метод `HEAD` этим и обслуживается, тело не скачивается. Дальше тело тянется порциями по 8 МБ, так что таймаут HTTP-клиента расходуется на порцию, а не на весь файл. Размер порции дополнительно зажимается значением [`client_max_body_size`](/config#client-max-body-size): ответ крупнее него клиент отвергает.

* `Range` клиента вырезает сам S3 — с объекта в сотни мегабайт качается только запрошенное. Открытый конец (`bytes=100-`) клипуется размером порции: RFC 9110 §14.2 разрешает отдать подмножество запрошенного, и плеер продолжит конкретными диапазонами.
* Несколько диапазонов в одном заголовке тянут объект целиком, а режет его дальше обычный механизм — ответ приходит как `multipart/byteranges`.
* `If-None-Match` и `If-Modified-Since` проксируются в S3, его `304` ретранслируется без похода за телом. `If-Range` сверяется с `ETag` локально.
* `gzip` к S3-ветке не применяется: `gzip_static` и кеш сжатого — локальная инфраструктура, а видео в `main.gzip` и так не входит.
* `403` от S3 отдаётся клиенту как `502`: это проблема ключей сервера, а не прав клиента на объект. Таймаут и недоступность — `503`.

Порция S3 занимает рабочий поток из общего пула `main.threads` — того же, из которого работают обработчики. На узком пуле медленный S3 будет тормозить весь виртуальный хост.

::: warning Ключ storage требует static_file
`storage` без `static_file` — ошибка конфигурации, как и `storage` вместе с `file`/`function`. Неизвестное имя хранилища тоже не даст серверу подняться: имя проверяется по секции `storages` при загрузке конфигурации.
:::

### Комбинирование с обработчиками

Статические файлы и обработчики можно комбинировать в одном маршруте для разных методов:

```json
"/api/docs": {
    "GET":  { "static_file": "public/api-docs.html" },
    "POST": { "file": "handlers/libapi.so", "function": "update_docs" }
}
```

### Rate limiting для статических файлов

На `static_file` из `root` действует `ratelimit` маршрута, а без него — `http.ratelimit`. Проверка выполняется до поиска файла: запрос к несуществующему файлу тоже расходует токен. При исчерпанном лимите возвращается `429` с `Retry-After: 1`; `cache_control` маршрута на этот ответ не применяется.

Запрос, не совпавший ни с одним маршрутом, также проверяется по `http.ratelimit` до поиска файла в `root`. Такие запросы и маршруты без собственного профиля расходуют общий бакет клиента: серия запросов к отсутствующим файлам может исчерпать лимит и для существующего файла. После пополнения бакета отсутствующий файл снова получает обычный `404`.

Если профиль не назначен ни маршруту, ни `http`, ограничения нет. Профиль маршрута с `rate: 0` отключает ограничение для него, даже если установлен `http.ratelimit`. Middleware на статике из `root` по-прежнему не выполняются — rate limiting работает независимо от них.

Профиль можно назначить непосредственно файловому маршруту:

```json
"/downloads/(.*)": {
    "GET": {
        "static_file": "{1}",
        "ratelimit": "downloads"
    }
}
```

::: tip Когда использовать static_file
Используйте `static_file` для файлов, которые не требуют обработки: HTML-страницы, изображения, документы, favicon и т.д. Это эффективнее, чем создавать обработчик для каждого файла.
:::

## Middleware

Middleware выполняются перед обработчиком и применяются ко всем маршрутам секции (`http.middlewares` / `websockets.middlewares`). В HTTP это маршруты с обработчиком и [storage-маршруты](#отдача-из-хранилища); статика от `root` — и через `static_file`, и запрос, не совпавший ни с одним маршрутом, — отдаётся без middleware:

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
        "strict":  { "burst": 1,   "rate": 1 },
        "api":     { "burst": 100, "rate": 100 }
    },
    "http": {
        "ratelimit": "default",
        "routes": {
            "/api/users": {
                "GET":  { "file": "handlers/libapi.so", "function": "get_users",   "ratelimit": "api" },
                "POST": { "file": "handlers/libapi.so", "function": "create_user", "ratelimit": "api" }
            },
            "/api/login": {
                "POST": { "file": "handlers/libapi.so", "function": "login", "ratelimit": "strict" }
            }
        }
    }
}
```

* `http.ratelimit` — профиль по умолчанию для всех HTTP-маршрутов и статики от `root` по запросу, не совпавшему ни с одним маршрутом. Маршруты без собственного профиля и такая статика делят один бакет на адрес клиента.
* `ratelimit` в конкретном маршруте переопределяет профиль для него. Профиль задаётся внутри метода, но действует на весь маршрут; для разных лимитов используйте отдельные маршруты.
* Ограничение действует и на `static_file` из `root`, и на [storage-маршруты](#отдача-из-хранилища). Запросы к отсутствующим файлам также расходуют токены. При исчерпании лимита HTTP возвращает `429` с `Retry-After: 1`.
* Редиректы из `http.redirects` обрабатываются до маршрутизации и не расходуют токены этого лимитера.
* Бакет свой у каждого адреса клиента. `rate: 0` выключает профиль — ограничитель пропускает все запросы; самый строгий профиль — `{ "burst": 1, "rate": 1 }`, см. [ratelimits](/config#ratelimits).

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

Локация редиректа **не якорится** и сравнивается с путём запроса без строки запроса: `"/user": "/persons"` срабатывает на любом пути, где встречается `/user`, — в том числе на `/api/user/42`. Чтобы редирект отвечал только на точный путь, поставьте якоря: `"^/user$"`.

В локации редиректа, как и в шаблоне маршрута, — не больше 64 групп захвата.

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

### Статика в хранилищах

Вёрстка — в каталоге на диске, видео — в S3, скачивания — за авторизацией и с ограничением частоты:

```json
"storages": {
    "assets": { "type": "filesystem", "root": "/var/www/assets" },
    "media": {
        "type": "s3", "access_id": "...", "access_secret": "...",
        "protocol": "https", "host": "s3.example.com", "port": "",
        "bucket": "media", "region": "us-east-1"
    }
},
"servers": {
    "s1": {
        "ratelimits": { "downloads": { "burst": 5, "rate": 1 } },
        "http": {
            "middlewares": ["middleware_auth"],
            "routes": {
                "/assets/(.*)": {
                    "GET": {
                        "static_file": "{1}",
                        "storage": "assets",
                        "cache_control": "public, max-age=31536000, immutable"
                    }
                },
                "/videos/(.*)": {
                    "GET":  { "static_file": "{1}", "storage": "media", "ratelimit": "downloads" },
                    "HEAD": { "static_file": "{1}", "storage": "media" }
                }
            }
        }
    }
}
```

`middleware_auth` здесь выполняется и для `/assets/`, и для `/videos/`: список middleware общий для вхоста. Middleware, которая должна закрывать только видео, проверяет путь запроса сама.

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
