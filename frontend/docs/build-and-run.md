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
./exec/cwfr -c /path/to/config.json
```

## Способы сборки

Ядро и приложение можно собирать вместе или порознь. Три варианта, от простого к развязанному:

| | Что собирается | Когда нужен |
|---|---|---|
| **1. Монорепозиторий** | ядро + приложение одной командой | разработка, CI, знакомство с фреймворком |
| **2. Приложение отдельно** | только приложение, против установленного фреймворка | ядро собрано один раз, обработчики пишутся и пересобираются потом |
| **3. Только фреймворк** | ядро без всякого приложения | подготовка того самого установленного фреймворка |

Способы 3 и 2 — две половины одного рабочего процесса: собрали ядро однажды, дальше работаете только с приложением.

### 1. Монорепозиторий

То, что делает [Быстрый старт](#быстрыи-старт): `backend/CMakeLists.txt` подключает `core/` и `app/` и собирает всё вместе.

### 2. Приложение отдельно

`backend/app/` — самостоятельный CMake-проект. Ему достаточно установленного фреймворка; исходники ядра не нужны:

```bash
cmake -S backend/app -B build -DCMAKE_BUILD_TYPE=Release \
      -Dcwfr_DIR=/opt/cwfr/lib/cmake/cwfr
cmake --build build -j$(nproc)
cmake --install build --prefix /srv/myapp
```

`-Dcwfr_DIR=…` нужен, только если фреймворк установлен в префикс, который CMake не просматривает сам; при `--prefix /usr/local` хватит `find_package(cwfr)`.

`find_package(cwfr 1.0 REQUIRED)` даёт:

* **`cwfr::framework`** — библиотеку вместе с путями к заголовкам и макросами, с которыми был собран фреймворк (`PCRE2_CODE_UNIT_WIDTH`, `PostgreSQL_FOUND`, …). Эти макросы управляют полями структур в заголовках БД, поэтому набор драйверов навязывается тот же — выбрать другой приложение не может;
* **функции сборки** — `cwfr_add_lib()`, `cwfr_add_handlers()`, `cwfr_add_migrations()`, `cwfr_add_subdirs()`, `cwfr_install_handlers()`, `cwfr_install_migrations()`.

Сверка идёт по мажорной версии, так что приложение откажется настраиваться против несовместимого выпуска ядра.

::: tip Один и тот же каталог собирается двумя способами
`backend/app/` работает и как часть монорепозитория (`add_subdirectory(app)`), и самостоятельно. Разница — один блок `if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)` в его `CMakeLists.txt`, где задаётся то, что в первом случае приходит от `backend/CMakeLists.txt`.
:::

### 3. Только фреймворк

Ядро не собирается само по себе: оно не вызывает `project()` и не ищет зависимости — это делает объемлющий проект. Минимальный такой проект состоит из преамбулы и одной строки `add_subdirectory(core)`:

```cmake
cmake_minimum_required(VERSION 3.12.4)
project(cwfr_framework_only LANGUAGES C)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/core/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/exec")

add_compile_options(-fPIC)
add_link_options(-rdynamic)

find_package(Threads REQUIRED)
find_package(PCRE2 REQUIRED)
add_definitions(-DPCRE2_CODE_UNIT_WIDTH=8)
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(IDN2 REQUIRED)
find_package(UNISTRING REQUIRED)

if(INCLUDE_POSTGRESQL STREQUAL yes)
    find_package(PostgreSQL)
endif()
if(PostgreSQL_FOUND AND INCLUDE_POSTGRESQL STREQUAL yes)
    add_definitions(-DPostgreSQL_FOUND)
endif()

add_subdirectory(core)
```

```bash
cmake -S fwonly -B build -DCMAKE_BUILD_TYPE=Release -DINCLUDE_POSTGRESQL=yes
cmake --build build -j$(nproc)
cmake --install build --prefix /opt/cwfr
```

Всё остальное ядро объявляет само — включая `install()`-правила и CMake-пакет, который потом найдёт способ 2.

## Зависимости

Сборка требует установленных библиотек разработки (ищутся через `find_package`): **Threads**, **PCRE2**, **ZLIB**, **OpenSSL**, **LibXML2**, **libidn2**, **libunistring**. Поддержка БД включается по желанию (см. ниже) и требует соответствующих клиентов: **PostgreSQL**, **MySQL/MariaDB**, **hiredis** (Redis), **SQLite3**.

Пример установки в Ubuntu/Debian:

```bash
sudo apt install build-essential cmake pkg-config \
                 libpcre2-dev zlib1g-dev libssl-dev libxml2-dev \
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

### Протоколы

```bash
-DINCLUDE_HTTP3=yes        # HTTP/3 и QUIC (требует OpenSSL ≥ 3.5)
```

HTTP/3 выключен по умолчанию, поскольку QUIC требует QUIC TLS API из OpenSSL 3.5+. Остальная часть фреймворка совместима с OpenSSL 1.1.1+, поэтому этот флаг — единственный компонент с повышенным требованием. Включение флага проверяет, что libssl реально экспортирует QUIC API (некоторые дистрибутивы собраны с `no-quic`). HTTP/2 всегда входит в сборку и отдельного флага не требует. Подробнее — в разделе [HTTP/3](/http3).

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
│   │   ├── server/                # → cwfr (main.c)
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
│   ├── app_init.c                 # Точка входа модуля приложения
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
├── cwfr                           # Основной исполняемый файл
├── migrate                        # Утилита миграций
├── libapp.so                      # Модуль приложения (main.modules)
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

Отдельно стоит `libapp.so` — код приложения (модели, middleware, контексты) одной разделяемой библиотекой. Сервер загружает её по пути из [`main.modules`](/config#modules) и вызывает `app_init()`; обработчики резолвят её символы из этого единственного экземпляра. Библиотека ядра, `libcwfr_framework.so`, не содержит ничего прикладного — потому её и можно собрать один раз и переиспользовать.

Каталоги обработчиков и миграций переносятся независимо от остальной сборки:

```bash
cmake .. -DCWFR_HANDLER_OUT_DIR=$HOME/handlers \
         -DCWFR_MIGRATION_OUT_DIR=$HOME/migrations
```

## Установка

```bash
cmake --install build --prefix /opt/cwfr
```

```
/opt/cwfr/
├── bin/
│   ├── cwfr                       # сервер
│   └── migrate                    # утилита миграций
├── include/cwfr/                  # публичные заголовки, плоско
└── lib/
    ├── cwfr/
    │   ├── libcwfr_framework.so   # + .so.1 и .so.1.0.0
    │   ├── libapp.so
    │   ├── handlers/
    │   └── migrations/
    └── cmake/cwfr/                # пакет для find_package(cwfr)
```

`cwfr` и `migrate` несут `INSTALL_RPATH` вида `$ORIGIN/../lib/cwfr`, поэтому дерево переносимо — ни `ldconfig`, ни `LD_LIBRARY_PATH` не нужны, пока сохраняется взаимное расположение `bin/` и `lib/cwfr/`.

Заголовки кладутся **плоско**, в один каталог: исходники ядра включают друг друга по коротким именам (`"httprequest.h"`, а не `"protocols/http/httprequest.h"`), и плоская раскладка воспроизводит это одним `-I`, не вынося наружу внутреннее устройство ядра.

У `libcwfr_framework.so` есть `SONAME` с мажорной версией. Обработчики её запоминают, поэтому несовместимое обновление ядра обернётся внятной ошибкой при загрузке, а не порчей данных на ходу.

Пути установки обработчиков и миграций переопределяются отдельно от префикса — абсолютный путь игнорирует `--prefix`:

```bash
cmake .. -DCWFR_HANDLER_INSTALL_DIR=/srv/myapp/handlers \
         -DCWFR_MIGRATION_INSTALL_DIR=/srv/myapp/migrations
```

## Запуск

```bash
# Запуск с указанием конфигурационного файла
./build/exec/cwfr -c /path/to/config.json

# Остаться на переднем плане (нужно контейнерам и супервизорам)
./build/exec/cwfr -c /path/to/config.json -f
```

Приложение запускается и слушает порты, заданные в `config.json`. Подробности — в разделе [Конфигурация](./config.md).

Сборка `Release` отделяется от терминала, если не передан `-f`. Флаг `-f` нужен там, где процесс наблюдает супервизор: для него процесс, который сделал fork и вышел, выглядит как упавший.

**Код возврата осмыслен в обоих режимах.** Процесс не сообщает об успехе, пока конфигурация не прочитана, проверена и применена, **а все воркеры не начали слушать**. Поэтому `cwfr -c config.json && ...` работает так, как написано: и отвергнутый конфиг, и сокет, который не удалось привязать, дают ненулевой код и не оставляют висящего процесса. Отделяющийся родитель ждёт этого момента, а значит к возврату команды сервер уже принимает соединения.

## Горячая перезагрузка

Сигнал `SIGUSR1` перезагружает конфигурацию (`config.json`) без остановки сервера:

```bash
pkill -USR1 cwfr
```
