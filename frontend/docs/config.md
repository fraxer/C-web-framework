---
outline: deep
description: Полное описание конфигурации C Web Framework. Настройка серверов, баз данных, middleware, rate limiting, хранилищ, сессий и email.
---

# Файл конфигурации

Конфигурация приложения хранится в файле `config.json`. Файл содержит настройки серверов, баз данных, хранилищ, сессий, email и других компонентов.

## Секция main

Основные настройки приложения.

### workers <Badge type="info" text="число"/>

Количество worker-потоков для обработки соединений и чтения/записи данных.

### threads <Badge type="info" text="число"/>

Количество потоков для обработки запросов и формирования ответов.

### reload <Badge type="info" text="soft | hard"/>

Режим горячей перезагрузки:

* `soft` — перезагрузка с сохранением активных соединений
* `hard` — перезагрузка с принудительным закрытием соединений

### buffer_size <Badge type="info" text="число"/>

Размер буфера для чтения и записи данных в сокет (в байтах).

### client_max_body_size <Badge type="info" text="число"/>

Максимальный размер тела запроса (в байтах).

### tmp <Badge type="info" text="строка"/>

Путь до директории временных файлов.

### gzip <Badge type="info" text="массив строк"/>

Список MIME-типов для автоматического сжатия. Оставьте пустым, если сжатие не требуется.

## Секция migrations

### source_directory <Badge type="info" text="строка"/>

Путь до директории с файлами миграций.

## Секция servers

Конфигурация HTTP/WebSocket серверов. Каждый сервер имеет уникальный идентификатор (например, `s1`, `s2`).

### domains <Badge type="info" text="массив строк"/>

Список доменов, привязанных к серверу. Поддерживаются:

* Точные имена: `example.com`
* Подстановочные: `*.example.com`
* Регулярные выражения: `(api|www).example.com`

### ip <Badge type="info" text="строка"/>

IP-адрес для прослушивания, например `127.0.0.1`.

### port <Badge type="info" text="число"/>

Порт сервера (обычно `80` для HTTP, `443` для HTTPS).

### root <Badge type="info" text="строка"/>

Путь до корневой директории статических файлов.

### ratelimits <Badge type="info" text="объект"/>

Настройки ограничения частоты запросов (Rate Limiting):

```json
"ratelimits": {
    "default": { "burst": 15, "rate": 15 },
    "strict": { "burst": 1, "rate": 0 }
}
```

* `burst` — максимальное количество запросов в пике
* `rate` — скорость восполнения токенов

### http <Badge type="info" text="объект"/>

Конфигурация HTTP-маршрутов и middleware.

#### ratelimit <Badge type="info" text="строка"/>

Имя профиля rate limiting по умолчанию для всех маршрутов.

#### middlewares <Badge type="info" text="массив строк"/>

Список middleware, применяемых ко всем маршрутам:

```json
"middlewares": ["middleware_auth", "middleware_log"]
```

#### routes <Badge type="info" text="объект"/>

Маршруты HTTP-запросов:

```json
"routes": {
    "/api/users": {
        "GET": {
            "file": "/path/to/handler.so",
            "function": "get_users",
            "ratelimit": "strict"
        },
        "POST": {
            "file": "/path/to/handler.so",
            "function": "create_user"
        }
    },
    "/api/users/{id|\\d+}": {
        "PATCH": {
            "file": "/path/to/handler.so",
            "function": "update_user"
        }
    }
}
```

Параметры маршрута:
* `file` — путь до .so файла с обработчиком
* `function` — имя функции-обработчика
* `ratelimit` — переопределение профиля rate limiting для маршрута

#### redirects <Badge type="info" text="объект"/>

Правила редиректов с поддержкой регулярных выражений:

```json
"redirects": {
    "/old-path": "/new-path",
    "/number/(\\d)/(\\d)": "/digit/{1}/{2}"
}
```

Группы захвата доступны через `{1}`, `{2}` и т.д.

### websockets <Badge type="info" text="объект"/>

Конфигурация WebSocket. Структура аналогична секции `http`:

```json
"websockets": {
    "default": {
        "file": "/path/to/ws_handler.so",
        "function": "default_handler"
    },
    "routes": {
        "/ws": {
            "GET": { "file": "/path/to/ws_handler.so", "function": "connect" },
            "POST": { "file": "/path/to/ws_handler.so", "function": "message" }
        }
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

Глобальная конфигурация баз данных. Поддерживается несколько подключений к каждому типу БД.

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

## Секция storages

Конфигурация файловых хранилищ.

### Локальное хранилище

```json
"storages": {
    "local": {
        "type": "filesystem",
        "root": "/var/www/storage"
    }
}
```

### S3-совместимое хранилище

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

## Секция sessions

Конфигурация сессий пользователей.

```json
"sessions": {
    "driver": "storage",
    "host_id": "redis.r1",
    "storage_name": "sessions",
    "lifetime": 3600
}
```

* `driver` — драйвер хранения сессий (`storage` или `file`)
* `host_id` — идентификатор хранилища для сессий
* `lifetime` — время жизни сессии в секундах

## Секция mail

Конфигурация отправки email.

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
        "buffer_size": 16384,
        "client_max_body_size": 110485760,
        "tmp": "/tmp",
        "gzip": ["text/html", "text/css", "application/json", "application/javascript"]
    },
    "migrations": {
        "source_directory": "/path/to/migrations"
    },
    "servers": {
        "s1": {
            "domains": ["example.com", "*.example.com"],
            "ip": "127.0.0.1",
            "port": 443,
            "root": "/var/www/html",
            "ratelimits": {
                "default": { "burst": 15, "rate": 15 },
                "strict": { "burst": 1, "rate": 0 }
            },
            "http": {
                "ratelimit": "default",
                "middlewares": ["middleware_auth"],
                "routes": {
                    "/api/users": {
                        "GET": { "file": "handlers/libapi.so", "function": "get_users" },
                        "POST": { "file": "handlers/libapi.so", "function": "create_user" }
                    }
                },
                "redirects": {
                    "/old": "/new"
                }
            },
            "websockets": {
                "routes": {
                    "/ws": {
                        "GET": { "file": "handlers/libws.so", "function": "connect" }
                    }
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
            "host_id": "p1",
            "ip": "127.0.0.1",
            "port": 5432,
            "dbname": "mydb",
            "user": "dbuser",
            "password": "dbpass",
            "connection_timeout": 3
        }],
        "redis": [{
            "host_id": "r1",
            "ip": "127.0.0.1",
            "port": 6379,
            "dbindex": 0,
            "user": "",
            "password": ""
        }]
    },
    "storages": {
        "files": {
            "type": "filesystem",
            "root": "/var/www/storage"
        }
    },
    "sessions": {
        "driver": "storage",
        "host_id": "redis.r1",
        "storage_name": "sessions",
        "lifetime": 3600
    },
    "mail": {
        "dkim_private": "/path/to/dkim_private.pem",
        "dkim_selector": "mail",
        "host": "example.com"
    },
    "mimetypes": {
        "text/html": ["html", "htm"],
        "text/css": ["css"],
        "application/json": ["json"],
        "application/javascript": ["js"],
        "image/png": ["png"],
        "image/jpeg": ["jpeg", "jpg"]
    }
}
```
