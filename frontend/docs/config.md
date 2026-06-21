---
outline: deep
description: Полное описание конфигурации C Web Framework. Настройка серверов, баз данных, middleware, rate limiting, хранилищ, сессий, задач и email.
---

# Файл конфигурации

Конфигурация приложения хранится в файле `config.json` (передаётся через `cpdy -c <config>`). Файл описывает серверы, базы данных, хранилища, сессии, планировщик задач, email и другие компоненты. Перезагрузить конфигурацию без остановки сервера можно сигналом `SIGUSR1` (`pkill -USR1 cpdy`) — поведение при перезагрузке задаётся параметром `main.reload`.

## Секция main

Основные настройки приложения.

### workers <Badge type="info" text="число"/>

Количество worker-процессов/потоков для приёма соединений и чтения/записи данных.

### threads <Badge type="info" text="число"/>

Количество потоков для обработки запросов и формирования ответов.

### reload <Badge type="info" text="soft | hard"/>

Режим горячей перезагрузки (по `SIGUSR1`):

* `soft` (по умолчанию) — перезагрузка с сохранением активных соединений
* `hard` — перезагрузка с принудительным закрытием соединений

### client_max_body_size <Badge type="info" text="число"/>

Максимальный размер тела запроса в байтах.

### tmp <Badge type="info" text="строка"/>

Путь до директории временных файлов.

### gzip <Badge type="info" text="массив строк"/>

Список MIME-типов для автоматического сжатия ответов. Оставьте пустым, если сжатие не требуется.

### log <Badge type="info" text="объект"/>

Настройки системы логирования:

```json
"log": {
    "enabled": true,
    "level": "info"
}
```

#### enabled <Badge type="info" text="boolean"/>

Включает или отключает логирование. При `false` все функции логирования игнорируются.

#### level <Badge type="info" text="строка"/>

Минимальный уровень логирования. Сообщения с более низким приоритетом отфильтровываются. Допустимые значения (от наиболее критичных к наименее критичным):

- `emerg` — система неработоспособна (0)
- `alert` — требуется немедленное действие (1)
- `crit` — критическое состояние (2)
- `err` / `error` — ошибки (3)
- `warning` / `warn` — предупреждения (4)
- `notice` — важные уведомления (5)
- `info` — информационные сообщения (6)
- `debug` — отладочные сообщения (7)

::: tip Рекомендации
- **Production:** `info` или `notice` — баланс между информативностью и производительностью.
- **Development:** `debug` — максимально детальное логирование.
- **Critical systems:** `warning`/`error` — только важные события.
:::

### env <Badge type="info" text="объект"/>

Произвольное пользовательское хранилище ключ-значение. Значения (`string`/`number`/`bool`/`null`) копируются как есть и доступны в коде через функции `env_get_*`:

```json
"env": {
    "refresh_token_expiration": 15552000,
    "feature_x_enabled": true,
    "app_name": "backend"
}
```

```c
long long ttl = env_get_llong("refresh_token_expiration", 3600);
int enabled  = env_get_bool("feature_x_enabled", 0);
const char* name = env_get_string("app_name", "default");
```

Доступны: `env_get_string`, `env_get_int`, `env_get_llong`, `env_get_bool`, `env_get_double`, `env_get_ldouble` — каждое принимает ключ и значение по умолчанию.

## Секция migrations

### source_directory <Badge type="info" text="строка"/>

Путь до директории с исходниками миграций (используется утилитой `migrate`).

## Секция translations

Список доменов локализации (i18n). Для каждого домена указывается текстовый домен и путь к каталогу `.mo`-файлов:

```json
"translations": [
    { "domain": "backend", "path": "/app/locale" }
]
```

* `domain` — имя текстового домена (обязательный)
* `path` — путь к каталогу переводов (обязательный)

## Секция task_manager

Список фоновых задач, загружаемых из `.so` и исполняемых планировщиком. Каждая задача — объект с общими полями `name`, `type`, `file`, `function` и полями расписания в зависимости от `type`.

| `type` | Поля расписания | Описание |
|--------|-----------------|----------|
| `interval` | `interval` | Запуск каждые N секунд (≥ 1) |
| `daily` | `hour`, `minute` | Ежедневно в `hh:mm` |
| `weekly` | `weekday`, `hour`, `minute` | Еженедельно в указанный день недели |
| `monthly` | `day`, `hour`, `minute` | Ежемесячно в указанный день месяца |

`weekday` принимает значения `sunday`…`saturday`; `hour` — 0–23, `minute` — 0–59, `day` — 1–31.

```json
"task_manager": [
    {
        "name": "cleanup_expired_tokens",
        "type": "interval",
        "interval": 60,
        "file": "/app/build/exec/handlers/tasks/libtasks.so",
        "function": "cleanup_authorization_codes"
    },
    {
        "name": "nightly_report",
        "type": "daily",
        "hour": 3,
        "minute": 30,
        "file": "/app/build/exec/handlers/tasks/libtasks.so",
        "function": "send_report"
    }
]
```

## Секция servers

Конфигурация HTTP/WebSocket серверов. Каждый сервер — именованная запись (`s1`, `s2`, …).

### domains <Badge type="info" text="массив строк"/>

Список доменов, привязанных к серверу. Поддерживаются:

* Точные имена: `example.com`
* Подстановочные: `*.example.com`, `mail.*`
* Регулярные выражения: `(api|www).example.com`, `(.1|.*|a3).example.com`

### ip <Badge type="info" text="строка"/>

IP-адрес для прослушивания, например `127.0.0.1`.

### port <Badge type="info" text="число"/>

Порт сервера (обычно `80` для HTTP, `443` для HTTPS).

### root <Badge type="info" text="строка"/>

Путь до корневой директории статических файлов.

### index <Badge type="info" text="строка"/>

Имя индексного файла, возвращаемого для каталогов (например, `index.html`).

### ratelimits <Badge type="info" text="объект"/>

Именованные профили ограничения частоты запросов (Rate Limiting). Каждый профиль задаёт `burst` (пиковое число запросов) и `rate` (скорость восполнения токенов):

```json
"ratelimits": {
    "one":   { "burst": 1,  "rate": 0  },
    "two":   { "burst": 15, "rate": 15 }
}
```

### http <Badge type="info" text="объект"/>

Конфигурация HTTP-маршрутов, middleware и редиректов.

#### ratelimit <Badge type="info" text="строка"/>

Имя профиля rate limiting по умолчанию для всех маршрутов секции.

#### middlewares <Badge type="info" text="массив строк"/>

Список middleware, применяемых ко всем маршрутам (имена из реестра middleware):

```json
"middlewares": ["middleware_http_auth"]
```

#### routes <Badge type="info" text="объект"/>

Маршруты HTTP-запросов. Ключ — путь (с поддержкой регулярных выражений и именованных параметров), значение — объект `METHOD → { обработчик }`:

```json
"routes": {
    "/api/users": {
        "GET":  { "file": "handlers/models/lib_modeluser.so", "function": "list", "ratelimit": "two" },
        "POST": { "file": "handlers/models/lib_modeluser.so", "function": "create" }
    },
    "/api/users/{id|\\d+}": {
        "PATCH": { "file": "handlers/models/lib_modeluser.so", "function": "update" }
    }
}
```

Параметры обработчика:

* `file` — путь до `.so` с обработчиком
* `function` — имя функции-обработчика
* `static_file` — путь к статическому файлу; если задано, запрос отдаёт файл вместо вызова `file`/`function`
* `ratelimit` — переопределение профиля rate limiting для маршрута

#### redirects <Badge type="info" text="объект"/>

Правила редиректов с поддержкой регулярных выражений. Группы захвата подставляются через `{1}`, `{2}`, …:

```json
"redirects": {
    "/user": "/persons",
    "/user(.*)/(\\d)": "/user-{1}-{2}",
    "/section1/(\\d+)/section2/(\\d+)/section3": "/one/{1}/two/{2}/three"
}
```

### websockets <Badge type="info" text="объект"/>

Конфигурация WebSocket. Поддерживает обработчик по умолчанию (`default`), общий `ratelimit`, `middlewares` и `routes`:

```json
"websockets": {
    "default": { "file": "handlers/ws/lib_wsindex.so", "function": "default_" },
    "routes": {
        "/": { "GET": { "file": "handlers/ws/lib_wsindex.so", "function": "echo" } }
    }
}
```

### tls <Badge type="info" text="объект"/>

Настройки TLS/SSL для HTTPS:

```json
"tls": {
    "fullchain": "/path/to/fullchain.pem",
    "private": "/path/to/privkey.pem",
    "ciphers": "TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256"
}
```

## Секция databases

Конфигурация подключений к БД. Каждый драйвер — массив хостов; в коде подключение адресуется как `<драйвер>.<host_id>` (например, `postgresql.p1`, `redis.r1`, `sqlite.local`). Компилируются только включённые при сборке драйверы (`-DINCLUDE_*`).

### postgresql

```json
"postgresql": [{
    "host_id": "p1",
    "ip": "127.0.0.1",
    "port": 5432,
    "dbname": "mydb",
    "user": "dbuser",
    "password": "dbpass",
    "connection_timeout": 3
}]
```

### mysql

```json
"mysql": [{
    "host_id": "m1",
    "ip": "127.0.0.1",
    "port": 3306,
    "dbname": "mydb",
    "user": "dbuser",
    "password": "dbpass"
}]
```

### redis

```json
"redis": [{
    "host_id": "r1",
    "ip": "127.0.0.1",
    "port": 6379,
    "dbindex": 0,
    "user": "",
    "password": ""
}]
```

### sqlite

```json
"sqlite": [{
    "host_id": "local",
    "path": "/var/lib/app/data.sqlite",
    "journal_mode": "WAL",
    "busy_timeout": 5000
}]
```

* `path` — путь к файлу БД (`":memory:"` для in-memory); обязательный
* `journal_mode` — `PRAGMA journal_mode` (по умолчанию `WAL`)
* `busy_timeout` — `PRAGMA busy_timeout` в миллисекундах (по умолчанию `5000`)

## Секция storages

Именованные файловые хранилища. Тип задаётся полем `type`.

### filesystem

```json
"storages": {
    "local": {
        "type": "filesystem",
        "root": "/var/www/storage"
    }
}
```

### s3

```json
"storages": {
    "remote": {
        "type": "s3",
        "access_id": "your_access_id",
        "access_secret": "your_access_secret",
        "protocol": "https",
        "host": "s3.amazonaws.com",
        "port": "443",
        "bucket": "my-bucket",
        "region": "us-east-1"
    }
}
```

`region` необязателен (по умолчанию определяется провайдером).

## Секция sessions

Именованный набор конфигураций сессий. Каждый ключ — имя сессии, используемое в коде (`session_create("backend", data, ttl)`). Все сессии шифруются **AES-256-GCM**; ключ выводится из обязательного поля `secret`.

Драйвер задаётся полем `driver`:

| `driver` | Поле хранения | Описание |
|----------|---------------|----------|
| `filesystem` | `storage_name` | Файлы в указанном хранилище (имя из секции `storages`) |
| `redis` | `host_id` | Redis, адресуется как `redis.<host_id>` |
| `database` | `host_id` | БД, адресуется как `<драйвер>.<host_id>` (например, `postgresql.p1`) |

```json
"sessions": {
    "backend": {
        "driver": "filesystem",
        "storage_name": "local",
        "secret": "change-me"
    },
    "scheduler": {
        "driver": "redis",
        "host_id": "redis.r1",
        "secret": "change-me"
    },
    "doc-editor": {
        "driver": "database",
        "host_id": "postgresql.p1",
        "secret": "change-me"
    }
}
```

Время жизни сессии не задаётся в конфигурации — оно передаётся при создании: `session_create(name, data, duration)`.

## Секция mail

Конфигурация отправки email с DKIM-подписью:

```json
"mail": {
    "dkim_private": "/path/to/dkim_private.pem",
    "dkim_selector": "mail",
    "host": "example.com"
}
```

## Секция mimetypes

Соответствие MIME-типов расширениям файлов:

```json
"mimetypes": {
    "text/html": ["html", "htm"],
    "text/css": ["css"],
    "application/json": ["json"],
    "application/javascript": ["js"],
    "image/png": ["png"],
    "image/jpeg": ["jpeg", "jpg"]
}
```

## Полный пример конфигурации

```json
{
    "main": {
        "workers": 4,
        "threads": 2,
        "reload": "hard",
        "client_max_body_size": 110485760,
        "tmp": "/tmp",
        "gzip": ["text/html", "text/css", "application/json", "application/javascript"],
        "log": { "enabled": true, "level": "info" },
        "env": { "refresh_token_expiration": 15552000 }
    },
    "migrations": {
        "source_directory": "/app/app/migrations"
    },
    "translations": [
        { "domain": "backend", "path": "/app/locale" }
    ],
    "task_manager": [
        {
            "name": "cleanup_expired_tokens",
            "type": "interval",
            "interval": 60,
            "file": "/app/build/exec/handlers/tasks/libtasks.so",
            "function": "cleanup_authorization_codes"
        }
    ],
    "servers": {
        "s1": {
            "domains": ["example.com", "*.example.com"],
            "ip": "127.0.0.1",
            "port": 443,
            "root": "/var/www/html",
            "index": "index.html",
            "ratelimits": {
                "default": { "burst": 15, "rate": 15 },
                "strict":  { "burst": 1,  "rate": 0  }
            },
            "http": {
                "ratelimit": "default",
                "middlewares": ["middleware_http_auth"],
                "routes": {
                    "/api/users": {
                        "GET":  { "file": "handlers/models/lib_modeluser.so", "function": "list" },
                        "POST": { "file": "handlers/models/lib_modeluser.so", "function": "create" }
                    },
                    "/robots.txt": {
                        "GET": { "static_file": "/var/www/html/robots.txt" }
                    }
                },
                "redirects": {
                    "/old": "/new"
                }
            },
            "websockets": {
                "default": { "file": "handlers/ws/lib_wsindex.so", "function": "default_" },
                "routes": {
                    "/ws": { "GET": { "file": "handlers/ws/lib_wsindex.so", "function": "connect" } }
                }
            },
            "tls": {
                "fullchain": "/path/to/fullchain.pem",
                "private": "/path/to/privkey.pem",
                "ciphers": "TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256"
            }
        }
    },
    "databases": {
        "postgresql": [{
            "host_id": "p1", "ip": "127.0.0.1", "port": 5432,
            "dbname": "mydb", "user": "dbuser", "password": "dbpass",
            "connection_timeout": 3
        }],
        "redis": [{
            "host_id": "r1", "ip": "127.0.0.1", "port": 6379,
            "dbindex": 0, "user": "", "password": ""
        }],
        "sqlite": [{
            "host_id": "local", "path": "/var/lib/app/data.sqlite"
        }]
    },
    "storages": {
        "local": { "type": "filesystem", "root": "/var/www/storage" }
    },
    "sessions": {
        "backend": {
            "driver": "filesystem",
            "storage_name": "local",
            "secret": "change-me"
        }
    },
    "mail": {
        "dkim_private": "/path/to/dkim_private.pem",
        "dkim_selector": "mail",
        "host": "example.com"
    },
    "mimetypes": {
        "text/html": ["html", "htm"],
        "application/json": ["json"],
        "application/javascript": ["js"],
        "image/png": ["png"]
    }
}
```
