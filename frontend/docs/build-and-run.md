---
outline: deep
description: Сборка C Web Framework из исходного кода. Режимы сборки Release и Debug, настройка CMake, структура проекта и запуск приложения.
---

# Сборка и запуск

## Быстрый старт

```bash
# Клонирование репозитория
git clone git@github.com:fraxer/C-web-framework.git
cd C-web-framework

# Создание директории сборки
mkdir build && cd build

# Настройка проекта с CMake
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DINCLUDE_POSTGRESQL=yes \
         -DINCLUDE_MYSQL=yes \
         -DINCLUDE_REDIS=yes \
         -DINCLUDE_SQLITE=yes

# Сборка проекта
cmake --build . -j$(nproc)

# Запуск приложения (config.json — в корне backend/)
./exec/cpdy -c /path/to/config.json
```

## Зависимости

Сборка требует установленных библиотек разработки (ищутся через `find_package`): **Threads**, **PCRE**, **ZLIB**, **OpenSSL**, **LibXML2**, **libidn2**, **libunistring**. Поддержка БД включается по желанию (см. ниже) и требует соответствующих клиентов: **PostgreSQL**, **MySQL/MariaDB**, **hiredis** (Redis), **SQLite3**.

Пример установки в Ubuntu/Debian:

```bash
sudo apt install build-essential cmake pkg-config \
                 libpcre3-dev zlib1g-dev libssl-dev libxml2-dev \
                 libidn2-dev libunistring-dev \
                 libpq-dev libmariadb-dev libhiredis-dev libsqlite3-dev
```

## Режимы сборки

Режим задаётся переменной `CMAKE_BUILD_TYPE`. Поведение режимов `Debug` и `RelWithDebInfo` настроено в корневом `CMakeLists.txt`.

| Режим | Описание |
|-------|----------|
| `Release` | Оптимизация производительности, без отладочной информации и санитайзеров. Рекомендуется для production |
| `Debug` | Отладочная информация, AddressSanitizer + детектор утечек, `-fanalyzer`, строгие предупреждения (`-Wall -Wextra -Wpedantic`), макрос `DEBUG` |
| `RelWithDebInfo` | Оптимизация **с** отладочной информацией; как и `Debug`, включает санитайзеры и строгие предупреждения |
| `MinSizeRel` | Оптимизация размера бинарника (стандартный режим CMake, без специфики фреймворка) |

::: tip Санитайзеры
`-fsanitize=address -fsanitize=leak -fanalyzer` включаются автоматически для `Debug` и `RelWithDebInfo`. Для production-сборки используйте `Release`.
:::

```bash
# Отладочная сборка
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DINCLUDE_POSTGRESQL=yes \
         -DINCLUDE_MYSQL=yes \
         -DINCLUDE_REDIS=yes \
         -DINCLUDE_SQLITE=yes

# Сборка с модульными тестами ядра (core/tests)
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=yes
cmake --build . -j$(nproc)
ctest --test-dir build
```

## Параметры CMake

### Базы данных

Поддержка БД подключается опционально — компилируется только то, что включено:

```bash
-DINCLUDE_POSTGRESQL=yes  # PostgreSQL (libpq)
-DINCLUDE_MYSQL=yes       # MySQL / MariaDB
-DINCLUDE_REDIS=yes       # Redis (hiredis)
-DINCLUDE_SQLITE=yes      # SQLite3
```

При `yes` фреймворк ищет соответствующую библиотеку через `find_package`; если она найдена, определяется макрос драйвера (`PostgreSQL_FOUND`, `MySQL_FOUND`, `Redis_FOUND`, `SQLite_FOUND`) и драйвер компилируется в ядро.

### Тесты

```bash
-DBUILD_TESTS=yes         # Включить модульные тесты (core/tests) и цель test/ctest
```

### Компилятор

Укажите конкретную версию компилятора при необходимости:

```bash
cmake .. -DCMAKE_C_COMPILER=/usr/bin/gcc-12
```

## Структура проекта

`backend/` — корень сборки (здесь находятся `CMakeLists.txt` и `config.json`). `core/` — ядро фреймворка (git-субмодуль), `app/` — пример приложения.

```
backend/
├── core/                          # Ядро фреймворка (субмодуль)
│   ├── apps/                      # Точка входа исполняемых файлов
│   │   ├── server/                # → cpdy (main.c)
│   │   └── migrate/               # → migrate (main.c)
│   ├── framework/                 # Компоненты фреймворка
│   │   ├── database/              # Слой БД (PostgreSQL, MySQL, Redis, SQLite)
│   │   ├── model/                 # ORM-система моделей
│   │   ├── session/               # Сессии (FS, Redis, БД; AES-256-GCM)
│   │   ├── storage/               # Хранилища (FS, S3)
│   │   ├── view/                  # Шаблонизатор
│   │   ├── middleware/            # Система middleware
│   │   ├── taskmanager/           # Планировщик фоновых задач
│   │   └── translation/           # i18n
│   ├── protocols/                 # Реализация протоколов
│   │   ├── http/                  # HTTP/1.1 сервер и клиент
│   │   ├── websocket/             # WebSocket
│   │   └── smtp/                  # SMTP клиент, DKIM
│   ├── src/                       # Среда выполнения
│   │   ├── server/                # HTTP-сервер, воркеры
│   │   ├── multiplexing/          # Epoll мультиплексирование
│   │   ├── thread/                # Пул потоков
│   │   ├── connection/            # Управление соединениями
│   │   ├── socket/                # Сокеты
│   │   ├── signal/                # Обработка сигналов (вкл. горячую перезагрузку)
│   │   ├── route/                 # Маршрутизация
│   │   ├── domain/                # Виртуальные хосты, regex, IDN
│   │   ├── config/                # Загрузка конфигурации
│   │   ├── mimetype/              # MIME-типы
│   │   ├── moduleloader/          # Динамическая загрузка .so
│   │   ├── ratelimiter/           # Ограничение частоты запросов
│   │   ├── openssl/               # Обёртки OpenSSL
│   │   └── broadcast/             # Broadcasting
│   ├── misc/                      # Утилиты (header-only)
│   │   ├── str.h                  # Динамические строки (SSO)
│   │   ├── array.h, hashmap.h, map.h  # Коллекции
│   │   ├── json.h                 # JSON парсер/генератор
│   │   ├── jwt.h, sha256.h, base64.h, uuid.h  # Крипто/кодирование
│   │   ├── query.h, queryparser.h # Разбор query-строк
│   │   ├── log.h                  # Логирование
│   │   └── gzip.h                 # Gzip
│   └── tests/                     # Модульные тесты ядра (BUILD_TESTS=yes)
│
├── app/                           # Пользовательское приложение
│   ├── routes/                    # HTTP/WebSocket обработчики (компилируются в .so)
│   │   ├── auth/                  # Аутентификация (login, registration, session)
│   │   ├── index/                 # Главная страница
│   │   ├── ws/                    # WebSocket обработчики
│   │   ├── models/                # API для моделей (modeluser, modeluserview)
│   │   ├── db/                    # Примеры работы с БД
│   │   ├── files/                 # Операции с файлами / хранилищем
│   │   ├── email/                 # Отправка email
│   │   ├── httpclient/            # HTTP-клиент
│   │   ├── json/                  # Примеры JSON
│   │   └── middleware/            # Примеры middleware
│   ├── models/                    # ORM-модели и view-модели
│   │   ├── user.c, userview.c
│   │   ├── role.c, permission.c
│   │   ├── user_role.c, role_permission.c
│   │   └── *view.c                # View-модели для JOIN-запросов
│   ├── middlewares/               # Пользовательские middleware
│   │   ├── httpmiddlewares.c      # HTTP middleware (auth и др.)
│   │   └── wsmiddlewares.c        # WebSocket middleware
│   ├── migrations/                # Миграции БД
│   │   ├── s1/                    # Миграции сервера s1
│   │   └── s2/                    # Миграции сервера s2
│   ├── broadcasting/              # Каналы broadcasting (mybroadcast)
│   ├── auth/                      # Модуль аутентификации
│   │   ├── auth.c                 # Хеширование, authenticate()
│   │   ├── password_validator.c   # Валидация паролей
│   │   └── email_validator.c      # Валидация email
│   ├── contexts/                  # Контексты запросов
│   │   ├── httpctx.c              # HTTP-контекст
│   │   └── wsctx.c                # WebSocket-контекст
│   └── views/                     # Шаблоны (.tpl)
│       ├── index.tpl
│       └── header.tpl
│
└── config.json                    # Конфигурация приложения
```

## Результаты сборки

После сборки исполняемые файлы и библиотеки находятся в `build/exec`:

```
build/exec/
├── cpdy                           # Основной исполняемый файл
├── migrate                        # Утилита миграций
├── handlers/                      # Скомпилированные обработчики (.so)
│   ├── index/lib_index.so         #   lib_<имя_файла>.so в подпапке группы
│   ├── ws/lib_wsindex.so
│   ├── auth/lib_auth.so
│   └── ...
└── migrations/                    # Скомпилированные миграции (.so)
    └── s1/
        ├── lib2023-04-04_17-55-00_create_user_table.so
        └── ...
```

Обработчики компилируются по одному `.so` на файл исходника (`app/routes/<группа>/<имя>.c` → `handlers/<группа>/lib_<имя>.so`) и загружаются динамически во время выполнения.

## Запуск

```bash
# Запуск с указанием конфигурационного файла
./build/exec/cpdy -c /path/to/config.json
```

Приложение запускается и слушает порты, заданные в `config.json`. Подробности — в разделе [Конфигурация](./config.md).

## Горячая перезагрузка

Сигнал `SIGUSR1` перезагружает конфигурацию (`config.json`) без остановки сервера:

```bash
pkill -USR1 cpdy
```
