---
outline: deep
description: Building C Web Framework from source code. Release and Debug build modes, CMake configuration, project structure and application launch.
---

# Build and run

## Quick start

```bash
# Clone the repository
git clone git@github.com:fraxer/C-web-framework.git
cd C-web-framework/backend

# Create build directory
mkdir build && cd build

# Configure project with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DINCLUDE_POSTGRESQL=yes \
         -DINCLUDE_MYSQL=yes \
         -DINCLUDE_REDIS=yes \
         -DINCLUDE_SQLITE=yes

# Build the project
cmake --build . -j$(nproc)

# Run the application (config.json lives at the backend/ root)
./exec/cpdy -c ../config.json
```

## Dependencies

Building requires development headers for the libraries looked up via `find_package`: **Threads**, **PCRE**, **ZLIB**, **OpenSSL**, **LibXML2**, **libidn2**, **libunistring**. Database support is opt-in (see below) and needs the matching clients: **PostgreSQL**, **MySQL/MariaDB**, **hiredis** (Redis), **SQLite3**.

Example install on Ubuntu/Debian:

```bash
sudo apt install build-essential cmake pkg-config \
                 libpcre3-dev zlib1g-dev libssl-dev libxml2-dev \
                 libidn2-dev libunistring-dev \
                 libpq-dev libmariadb-dev libhiredis-dev libsqlite3-dev
```

## Build modes

The mode is set with `CMAKE_BUILD_TYPE`. The behavior of `Debug` and `RelWithDebInfo` is configured in the root `CMakeLists.txt`.

| Mode | Description |
|------|-------------|
| `Release` | Performance optimization, no debug info, no sanitizers. Recommended for production |
| `Debug` | Debug info, AddressSanitizer + leak detector, `-fanalyzer`, strict warnings (`-Wall -Wextra -Wpedantic`), `DEBUG` macro |
| `RelWithDebInfo` | Optimized **with** debug info; like `Debug`, enables sanitizers and strict warnings |
| `MinSizeRel` | Binary size optimization (standard CMake mode, no framework-specific behavior) |

::: tip Sanitizers
`-fsanitize=address -fsanitize=leak -fanalyzer` are enabled automatically for `Debug` and `RelWithDebInfo`. Use `Release` for production builds.
:::

```bash
# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DINCLUDE_POSTGRESQL=yes \
         -DINCLUDE_MYSQL=yes \
         -DINCLUDE_REDIS=yes \
         -DINCLUDE_SQLITE=yes

# Build with core unit tests (core/tests)
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=yes
cmake --build . -j$(nproc)
ctest --test-dir build
```

## CMake parameters

### Databases

Database support is opt-in — only enabled drivers are compiled:

```bash
-DINCLUDE_POSTGRESQL=yes  # PostgreSQL (libpq)
-DINCLUDE_MYSQL=yes       # MySQL / MariaDB
-DINCLUDE_REDIS=yes       # Redis (hiredis)
-DINCLUDE_SQLITE=yes      # SQLite3
```

When set to `yes`, the framework looks up the corresponding library via `find_package`; if found, the driver macro (`PostgreSQL_FOUND`, `MySQL_FOUND`, `Redis_FOUND`, `SQLite_FOUND`) is defined and the driver is compiled into the core.

### Tests

```bash
-DBUILD_TESTS=yes         # Enable unit tests (core/tests) and the test/ctest target
```

### Compiler

Specify a specific compiler version if necessary:

```bash
cmake .. -DCMAKE_C_COMPILER=/usr/bin/gcc-12
```

## Project structure

`backend/` is the build root (it holds `CMakeLists.txt` and `config.json`). `core/` is the framework core (a git submodule); `app/` is the example application.

```
backend/
├── core/                          # Framework core (submodule)
│   ├── apps/                      # Executable entry points
│   │   ├── server/                # → cpdy (main.c)
│   │   └── migrate/               # → migrate (main.c)
│   ├── framework/                 # Framework components
│   │   ├── database/              # DB layer (PostgreSQL, MySQL, Redis, SQLite)
│   │   ├── model/                 # ORM model system
│   │   ├── session/               # Sessions (FS, Redis, DB; AES-256-GCM)
│   │   ├── storage/               # Storage (FS, S3)
│   │   ├── view/                  # Template engine
│   │   ├── middleware/            # Middleware system
│   │   ├── taskmanager/           # Background task scheduler
│   │   └── translation/           # i18n
│   ├── protocols/                 # Protocol implementations
│   │   ├── http/                  # HTTP/1.1 server and client
│   │   ├── websocket/             # WebSocket
│   │   └── smtp/                  # SMTP client, DKIM
│   ├── src/                       # Runtime
│   │   ├── server/                # HTTP server, workers
│   │   ├── multiplexing/          # Epoll multiplexing
│   │   ├── thread/                # Thread pool
│   │   ├── connection/            # Connection management
│   │   ├── socket/                # Sockets
│   │   ├── signal/                # Signal handling (incl. hot reload)
│   │   ├── route/                 # Routing
│   │   ├── domain/                # Virtual hosts, regex, IDN
│   │   ├── config/                # Config loading
│   │   ├── mimetype/              # MIME types
│   │   ├── moduleloader/          # Dynamic .so loading
│   │   ├── ratelimiter/           # Rate limiting
│   │   ├── openssl/               # OpenSSL helpers
│   │   └── broadcast/             # Broadcasting
│   ├── misc/                      # Utilities (header-only)
│   │   ├── str.h                  # Dynamic strings (SSO)
│   │   ├── array.h, hashmap.h, map.h  # Collections
│   │   ├── json.h                 # JSON parser/generator
│   │   ├── jwt.h, sha256.h, base64.h, uuid.h  # Crypto/encoding
│   │   ├── query.h, queryparser.h # Query-string parsing
│   │   ├── log.h                  # Logging
│   │   └── gzip.h                 # Gzip
│   └── tests/                     # Core unit tests (BUILD_TESTS=yes)
│
├── app/                           # User application
│   ├── routes/                    # HTTP/WebSocket handlers (compiled to .so)
│   │   ├── auth/                  # Authentication (login, registration, session)
│   │   ├── index/                 # Main page
│   │   ├── ws/                    # WebSocket handlers
│   │   ├── models/                # Model API (modeluser, modeluserview)
│   │   ├── db/                    # Database examples
│   │   ├── files/                 # File / storage operations
│   │   ├── email/                 # Email sending
│   │   ├── httpclient/            # HTTP client
│   │   ├── json/                  # JSON examples
│   │   └── middleware/            # Middleware examples
│   ├── models/                    # ORM models and view models
│   │   ├── user.c, userview.c
│   │   ├── role.c, permission.c
│   │   ├── user_role.c, role_permission.c
│   │   └── *view.c                # View models for JOIN queries
│   ├── middlewares/               # Custom middlewares
│   │   ├── httpmiddlewares.c      # HTTP middleware (auth, etc.)
│   │   └── wsmiddlewares.c        # WebSocket middleware
│   ├── migrations/                # Database migrations
│   │   ├── s1/                    # Migrations for server s1
│   │   └── s2/                    # Migrations for server s2
│   ├── broadcasting/              # Broadcasting channels (mybroadcast)
│   ├── auth/                      # Authentication module
│   │   ├── auth.c                 # Hashing, authenticate()
│   │   ├── password_validator.c   # Password validation
│   │   └── email_validator.c      # Email validation
│   ├── contexts/                  # Request contexts
│   │   ├── httpctx.c              # HTTP context
│   │   └── wsctx.c                # WebSocket context
│   └── views/                     # Templates (.tpl)
│       ├── index.tpl
│       └── header.tpl
│
└── config.json                    # Application configuration
```

## Build results

After building, executables and libraries are placed in `build/exec`:

```
build/exec/
├── cpdy                           # Main executable
├── migrate                        # Migration utility
├── handlers/                      # Compiled handlers (.so)
│   ├── index/lib_index.so         #   lib_<file_name>.so under the group folder
│   ├── ws/lib_wsindex.so
│   ├── auth/lib_auth.so
│   └── ...
└── migrations/                    # Compiled migrations (.so)
    └── s1/
        ├── lib2023-04-04_17-55-00_create_user_table.so
        └── ...
```

Handlers are compiled one `.so` per source file (`app/routes/<group>/<name>.c` → `handlers/<group>/lib_<name>.so`) and loaded dynamically at runtime.

## Launch

```bash
# Run with a configuration file
./exec/cpdy -c ../config.json
```

The application starts and listens on the ports defined in `config.json`. See [Configuration](./config.md) for details.

## Hot reload

`SIGUSR1` reloads the configuration (`config.json`) without stopping the server:

```bash
pkill -USR1 cpdy
```
