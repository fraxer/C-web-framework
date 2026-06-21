---
outline: deep
description: Step-by-step C Web Framework installation on Linux. Setting up GCC, CMake, OpenSSL, PCRE, Zlib, LibXml2, libidn2, libunistring and connecting PostgreSQL, MySQL, Redis, SQLite.
---

# Install dependencies

The framework requires the following components to build and run:

**Required:** GCC 9.5+, CMake 3.12+, OpenSSL 1.1.1+, PCRE 8.43+, Zlib 1.2.11+, LibXml2 2.9.13+, libidn2 2.3.0+, libunistring 0.9.12+

**Optional:** PostgreSQL, MySQL/MariaDB, Redis, SQLite — install if you need to work with the corresponding databases.

::: tip Quick install (Ubuntu/Debian)
All required libraries and DB clients in a single command:
```bash
sudo apt install build-essential cmake pkg-config \
                 libpcre3-dev zlib1g-dev libssl-dev libxml2-dev \
                 libidn2-dev libunistring-dev \
                 libpq-dev libmariadb-dev libhiredis-dev libsqlite3-dev
```
You can then jump straight to [building the project](./build-and-run.md).
:::

Start by updating the package list:

```bash
sudo apt update
```

## GCC

Install the build-essential package:

```bash
sudo apt install build-essential
```

The command installs `gcc`, `g++` and `make`.

To verify that the GCC compiler has been successfully installed, use the `gcc --version` command, which displays the GCC version:

```bash
gcc --version
```

GCC is now installed on your system and you can start using it.

## CMake

### Package manager

Install cmake from the official repositories:

```bash
sudo apt install cmake
```

### Build from source

If your distro's repositories ship an outdated CMake, build it from source. Download the archive from the official site:

```bash
wget https://github.com/Kitware/CMake/releases/download/v3.27.0/cmake-3.27.0.tar.gz
```

Unpack:

```bash
tar -zxvf cmake-3.27.0.tar.gz
```

Change to the unpacked directory:

```bash
cd cmake-3.27.0
```

Start the build process:

```bash
./bootstrap
```

Compile and install:

```bash
make
sudo make install
```

## pkg-config

The `pkg-config` utility is used by the build system to locate headers and libraries (looked up via `find_package`):

```bash
sudo apt install pkg-config
```

## PCRE

Regular expression library (used for routing and regex-based virtual hosts):

```bash
sudo apt install libpcre3-dev
```

## Zlib

### Package manager

```bash
sudo apt install zlib1g-dev
```

### Build from source

Download the archive from the official site:

```bash
wget https://zlib.net/zlib-1.2.13.tar.gz
```

Unpack:

```bash
tar -zxvf zlib-1.2.13.tar.gz
```

Change to the unpacked directory:

```bash
cd zlib-1.2.13
```

Start the build process:

```bash
./configure
```

Compile and install:

```bash
make
sudo make install
```

## OpenSSL

TLS and cryptography library (HTTPS, AES-256-GCM sessions, JWT, etc.):

```bash
sudo apt install openssl libssl-dev
```

## LibXml2

### Package manager

```bash
sudo apt install libxml2-dev
```

### Build from source

Download the archive from the official site:

```bash
wget https://github.com/GNOME/libxml2/releases/download/v2.9.14/libxml2-2.9.14.tar.gz
```

Unpack:

```bash
tar -zxvf libxml2-2.9.14.tar.gz
```

Change to the unpacked directory:

```bash
cd libxml2-2.9.14
```

Start the build process:

```bash
./configure
```

Compile and install:

```bash
make
sudo make install
```

## libidn2

Library for internationalized domain name (IDN) support:

```bash
sudo apt install libidn2-dev
```

## libunistring

Unicode string library (required for IDN and i18n):

```bash
sudo apt install libunistring-dev
```

## Databases

Database support is enabled opt-in through CMake parameters (`-DINCLUDE_POSTGRESQL=yes`, `-DINCLUDE_MYSQL=yes`, `-DINCLUDE_REDIS=yes`, `-DINCLUDE_SQLITE=yes`). Building a driver requires its development client library (`-dev`), while running requires the server itself.

### PostgreSQL

Client library to build the driver (libpq):

```bash
sudo apt install libpq-dev
```

PostgreSQL server:

```bash
sudo apt install postgresql postgresql-contrib
```

Detailed instructions are available on [DigitalOcean](https://www.digitalocean.com/community/tutorials/how-to-install-postgresql-on-ubuntu-20-04-quickstart).

### MySQL / MariaDB

Client library to build the driver:

```bash
sudo apt install libmariadb-dev
```

MySQL server:

```bash
sudo apt install mysql-server
```

Detailed instructions are available on [DigitalOcean](https://www.digitalocean.com/community/tutorials/how-to-install-mysql-on-ubuntu-20-04).

### Redis

hiredis client library to build the driver:

```bash
sudo apt install libhiredis-dev
```

Redis server. Add the official repository to the apt index:

```bash
curl -fsSL https://packages.redis.io/gpg | sudo gpg --dearmor -o /usr/share/keyrings/redis-archive-keyring.gpg

echo "deb [signed-by=/usr/share/keyrings/redis-archive-keyring.gpg] https://packages.redis.io/deb $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/redis.list
```

Then install:

```bash
sudo apt update
sudo apt install redis
```

Detailed instructions are available on [Redis](https://redis.io/docs/getting-started/installation/install-redis-on-linux/).

### SQLite

Client library to build the driver:

```bash
sudo apt install libsqlite3-dev
```

SQLite works directly with a file — no separate server is required. If needed, install the command-line client:

```bash
sudo apt install sqlite3
```

## Next steps

Once the dependencies are installed, proceed to [building and running the project](./build-and-run.md).
