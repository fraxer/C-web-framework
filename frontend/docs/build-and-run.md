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
         -DINCLUDE_REDIS=yes

# Сборка проекта
cmake --build . -j4

# Запуск приложения
./exec/cpdy -c /path/to/config.json
```

## Режимы сборки

| Режим | Описание |
|-------|----------|
| `Release` | Оптимизация производительности, без отладочной информации. Рекомендуется для production |
| `Debug` | Отладочная информация включена, AddressSanitizer для поиска ошибок памяти |
| `RelWithDebInfo` | Оптимизация производительности с отладочной информацией |
| `MinSizeRel` | Оптимизация размера бинарных файлов |

```bash
# Сборка в режиме Debug
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DINCLUDE_POSTGRESQL=yes \
         -DINCLUDE_MYSQL=yes \
         -DINCLUDE_REDIS=yes
```

## Параметры CMake

### Базы данных

Включение поддержки баз данных:

```bash
-DINCLUDE_POSTGRESQL=yes  # Поддержка PostgreSQL
-DINCLUDE_MYSQL=yes       # Поддержка MySQL
-DINCLUDE_REDIS=yes       # Поддержка Redis
```

### Компилятор

Укажите конкретную версию компилятора при необходимости:

```bash
cmake .. -DCMAKE_C_COMPILER=/usr/bin/gcc-12
```

## Структура проекта

```
project/
├── core/                          # Ядро фреймворка
│   ├── framework/                 # Компоненты фреймворка
│   │   ├── database/             # Работа с БД (PostgreSQL, MySQL, Redis)
│   │   ├── model/                # ORM-система моделей
│   │   ├── session/              # Система сессий
│   │   ├── storage/              # Файловое хранилище (FS, S3)
│   │   ├── view/                 # Шаблонизатор
│   │   ├── middleware/           # Система middleware
│   │   └── query/                # Query builder
│   ├── protocols/                # Реализация протоколов
│   │   ├── http/                 # HTTP/1.1 сервер и клиент
│   │   ├── websocket/            # WebSocket сервер
│   │   └── smtp/                 # SMTP клиент, DKIM
│   ├── src/                      # Основные компоненты
│   │   ├── broadcast/            # Система broadcasting
│   │   ├── connection/           # Управление соединениями
│   │   ├── server/               # HTTP-сервер
│   │   ├── route/                # Система маршрутизации
│   │   ├── thread/               # Многопоточность
│   │   └── multiplexing/         # Epoll мультиплексирование
│   └── misc/                     # Утилиты
│       ├── str.h                 # Динамические строки
│       ├── array.h               # Массивы
│       ├── hashmap.h/map.h       # Ассоциативные массивы
│       ├── json.h                # JSON парсер
│       ├── log.h                 # Логирование
│       └── gzip.h                # Gzip сжатие
│
└── app/                          # Пользовательское приложение
    ├── routes/                   # HTTP и WebSocket обработчики
    │   ├── auth/                # Аутентификация (логин, регистрация)
    │   ├── index/               # Главная страница
    │   ├── ws/                  # WebSocket обработчики
    │   ├── files/               # Операции с файлами
    │   ├── models/              # API операции с моделями
    │   ├── email/               # Отправка email
    │   └── db/                  # Примеры работы с БД
    ├── models/                   # Модели данных
    │   ├── user.c               # Модель пользователя
    │   ├── role.c               # Модель роли
    │   ├── permission.c         # Модель разрешения
    │   └── *view.c              # View-модели для JOIN запросов
    ├── middlewares/              # Пользовательские middleware
    │   ├── httpmiddlewares.c    # HTTP middleware (auth, rate limit)
    │   └── wsmiddlewares.c      # WebSocket middleware
    ├── migrations/               # Миграции БД
    │   ├── s1/                  # Миграции для сервера s1
    │   └── s2/                  # Миграции для сервера s2
    ├── broadcasting/             # Каналы broadcasting
    │   └── mybroadcast.c        # Пример канала broadcasting
    ├── auth/                     # Модуль аутентификации
    │   ├── auth.c               # Функции аутентификации
    │   ├── password_validator.c # Валидация паролей
    │   └── email_validator.c    # Валидация email
    ├── contexts/                 # Контексты запросов
    │   ├── httpcontext.c        # HTTP контекст
    │   └── wscontext.c          # WebSocket контекст
    └── views/                    # Шаблоны
        ├── index.tpl            # Главная страница
        └── header.tpl           # Заголовок

config.json                       # Конфигурация приложения
```

## Результаты сборки

После сборки файлы располагаются в директории `build/exec`:

```
build/exec/
├── cpdy                          # Основной исполняемый файл
├── migrate                       # Утилита миграций
├── handlers/                     # Скомпилированные обработчики
│   ├── libindexpage.so
│   ├── libwsindexpage.so
│   └── ...
└── migrations/                   # Скомпилированные миграции
    ├── s1/
    │   └── lib2023-04-04_17-55-00_create_users_table.so
    └── ...
```

## Запуск

```bash
# Запуск с указанием конфигурационного файла
./build/exec/cpdy -c /path/to/config.json
```

Приложение запустится и начнет слушать указанные в конфигурации порты.

## Горячая перезагрузка

Для применения изменений конфигурации без остановки сервера:

```bash
pkill -USR1 cpdy
```
