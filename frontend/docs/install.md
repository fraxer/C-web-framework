---
outline: deep
description: Пошаговая установка C Web Framework на Linux. Настройка GCC, CMake, OpenSSL, PCRE, Zlib, LibXml2, libidn2, libunistring и подключение PostgreSQL, MySQL, Redis, SQLite.
---

# Установка зависимостей

Для сборки и работы фреймворка требуются следующие компоненты:

**Обязательные:** GCC 9.5+, CMake 3.12+, OpenSSL 1.1.1+, PCRE 8.43+, Zlib 1.2.11+, LibXml2 2.9.13+, libidn2 2.3.0+, libunistring 0.9.12+

**Опциональные:** PostgreSQL, MySQL/MariaDB, Redis, SQLite — устанавливаются при необходимости работы с соответствующими базами данных.

::: tip Быстрая установка (Ubuntu/Debian)
Все обязательные библиотеки и клиенты БД одной командой:
```bash
sudo apt install build-essential cmake pkg-config \
                 libpcre3-dev zlib1g-dev libssl-dev libxml2-dev \
                 libidn2-dev libunistring-dev \
                 libpq-dev libmariadb-dev libhiredis-dev libsqlite3-dev
```
После этого можно переходить к [сборке проекта](./build-and-run.md).
:::

Начните с обновления списка пакетов:

```bash
sudo apt update
```

## GCC

Установите пакет build-essential:

```bash
sudo apt install build-essential
```

Команда устанавливает `gcc`, `g++` и `make`.

Чтобы убедиться, что компилятор GCC успешно установлен, используйте команду `gcc --version`, которая выводит версию GCC:

```bash
gcc --version
```

Теперь GCC установлен в вашей системе, и вы можете начать его использовать.

## CMake

### Менеджер пакетов

Установка cmake из официальных репозиториев:

```bash
sudo apt install cmake
```

### Сборка из исходных файлов

Если в репозиториях дистрибутива старая версия CMake, соберите его из исходников. Скачайте архив с официального сайта:

```bash
wget https://github.com/Kitware/CMake/releases/download/v3.27.0/cmake-3.27.0.tar.gz
```

Распакуйте:

```bash
tar -zxvf cmake-3.27.0.tar.gz
```

Перейдите в распакованную директорию:

```bash
cd cmake-3.27.0
```

Запустите процесс сборки:

```bash
./bootstrap
```

Скомпилируйте и установите:

```bash
make
sudo make install
```

## pkg-config

Утилита `pkg-config` используется системой сборки для поиска заголовков и библиотек (ищутся через `find_package`):

```bash
sudo apt install pkg-config
```

## PCRE

Библиотека регулярных выражений (используется для маршрутизации и виртуальных хостов с regex):

```bash
sudo apt install libpcre3-dev
```

## Zlib

### Менеджер пакетов

```bash
sudo apt install zlib1g-dev
```

### Сборка из исходных файлов

Скачайте архив с официального сайта:

```bash
wget https://zlib.net/zlib-1.2.13.tar.gz
```

Распакуйте:

```bash
tar -zxvf zlib-1.2.13.tar.gz
```

Перейдите в распакованную директорию:

```bash
cd zlib-1.2.13
```

Запустите процесс сборки:

```bash
./configure
```

Скомпилируйте и установите:

```bash
make
sudo make install
```

## OpenSSL

Библиотека TLS и криптографии (HTTPS, сессии с AES-256-GCM, JWT и др.):

```bash
sudo apt install openssl libssl-dev
```

## LibXml2

### Менеджер пакетов

```bash
sudo apt install libxml2-dev
```

### Сборка из исходных файлов

Скачайте архив с официального сайта:

```bash
wget https://github.com/GNOME/libxml2/releases/download/v2.9.14/libxml2-2.9.14.tar.gz
```

Распакуйте:

```bash
tar -zxvf libxml2-2.9.14.tar.gz
```

Перейдите в распакованную директорию:

```bash
cd libxml2-2.9.14
```

Запустите процесс сборки:

```bash
./configure
```

Скомпилируйте и установите:

```bash
make
sudo make install
```

## libidn2

Библиотека для поддержки интернационализованных доменных имён (IDN):

```bash
sudo apt install libidn2-dev
```

## libunistring

Библиотека для работы с Unicode-строками (требуется для IDN и i18n):

```bash
sudo apt install libunistring-dev
```

## Базы данных

Поддержка баз данных включается опционально через параметры CMake (`-DINCLUDE_POSTGRESQL=yes`, `-DINCLUDE_MYSQL=yes`, `-DINCLUDE_REDIS=yes`, `-DINCLUDE_SQLITE=yes`). Для сборки соответствующего драйвера требуется клиентская библиотека разработки (`-dev`), а для запуска — сам сервер.

### PostgreSQL

Клиентская библиотека для сборки драйвера (libpq):

```bash
sudo apt install libpq-dev
```

Сервер PostgreSQL:

```bash
sudo apt install postgresql postgresql-contrib
```

Подробная инструкция представлена на [DigitalOcean](https://www.digitalocean.com/community/tutorials/how-to-install-postgresql-on-ubuntu-20-04-quickstart).

### MySQL / MariaDB

Клиентская библиотека для сборки драйвера:

```bash
sudo apt install libmariadb-dev
```

Сервер MySQL:

```bash
sudo apt install mysql-server
```

Подробная инструкция представлена на [DigitalOcean](https://www.digitalocean.com/community/tutorials/how-to-install-mysql-on-ubuntu-20-04).

### Redis

Клиентская библиотека hiredis для сборки драйвера:

```bash
sudo apt install libhiredis-dev
```

Сервер Redis. Добавьте официальный репозиторий в индекс apt:

```bash
curl -fsSL https://packages.redis.io/gpg | sudo gpg --dearmor -o /usr/share/keyrings/redis-archive-keyring.gpg

echo "deb [signed-by=/usr/share/keyrings/redis-archive-keyring.gpg] https://packages.redis.io/deb $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/redis.list
```

Затем установите:

```bash
sudo apt update
sudo apt install redis
```

Подробная инструкция представлена на [Redis](https://redis.io/docs/getting-started/installation/install-redis-on-linux/).

### SQLite

Клиентская библиотека для сборки драйвера:

```bash
sudo apt install libsqlite3-dev
```

Для работы с базой достаточно самого файла SQLite — отдельный сервер не требуется. При необходимости установите консольный клиент:

```bash
sudo apt install sqlite3
```

## Что дальше

После установки зависимостей переходите к [сборке и запуску проекта](./build-and-run.md).
