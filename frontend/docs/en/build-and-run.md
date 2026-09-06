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
./exec/cwfr -c ../config.json
```

## Build methods

The core and the application can be built together or apart. Three options, from the simplest to the most decoupled:

| | What gets built | When you want it |
|---|---|---|
| **1. Monorepo** | core + application in one command | development, CI, getting to know the framework |
| **2. Application alone** | just the application, against an installed framework | the core was built once; handlers are written and rebuilt afterwards |
| **3. Framework only** | the core with no application at all | producing that installed framework |

Methods 3 and 2 are two halves of one workflow: build the core once, then work only on the application.

### 1. Monorepo

What [Quick start](#quick-start) does: `backend/CMakeLists.txt` pulls in `core/` and `app/` and builds both.

### 2. Application alone

`backend/app/` is a CMake project in its own right. An installed framework is all it needs — the core sources are not:

```bash
cmake -S backend/app -B build -DCMAKE_BUILD_TYPE=Release \
      -Dcwfr_DIR=/opt/cwfr/lib/cmake/cwfr
cmake --build build -j$(nproc)
cmake --install build --prefix /srv/myapp
```

`-Dcwfr_DIR=…` is only needed when the framework sits in a prefix CMake does not search by default; with `--prefix /usr/local`, `find_package(cwfr)` finds it on its own.

`find_package(cwfr 1.0 REQUIRED)` provides:

* **`cwfr::framework`** — the library together with its header paths and the macros the framework was built with (`PCRE2_CODE_UNIT_WIDTH`, `PostgreSQL_FOUND`, …). Those macros gate struct members in the database headers, so the same set of drivers is imposed on the application — it does not get to pick a different one;
* **the build helpers** — `cwfr_add_lib()`, `cwfr_add_handlers()`, `cwfr_add_migrations()`, `cwfr_add_subdirs()`, `cwfr_install_handlers()`, `cwfr_install_migrations()`.

The match is by major version, so an application refuses to configure against a core release it was not written for.

::: tip One directory, two ways to build it
`backend/app/` works both as part of the monorepo (`add_subdirectory(app)`) and on its own. The difference is a single `if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)` block in its `CMakeLists.txt`, setting up what `backend/CMakeLists.txt` provides in the first case.
:::

### 3. Framework only

The core does not build standalone: it calls no `project()` and finds no dependencies — the enclosing project does. A minimal such project is a preamble plus one `add_subdirectory(core)`:

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

Everything else the core declares for itself — the `install()` rules included, and the CMake package that method 2 will go on to find.

## Dependencies

Building requires development headers for the libraries looked up via `find_package`: **Threads**, **PCRE2**, **ZLIB**, **OpenSSL**, **LibXML2**, **libidn2**, **libunistring**. Database support is opt-in (see below) and needs the matching clients: **PostgreSQL**, **MySQL/MariaDB**, **hiredis** (Redis), **SQLite3**.

Example install on Ubuntu/Debian:

```bash
sudo apt install build-essential cmake pkg-config \
                 libpcre2-dev zlib1g-dev libssl-dev libxml2-dev \
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

### Protocols

```bash
-DINCLUDE_HTTP3=yes        # HTTP/3 and QUIC (requires OpenSSL ≥ 3.5)
```

HTTP/3 is off by default, since QUIC requires the QUIC TLS API from OpenSSL 3.5+. The rest of the framework stays compatible with OpenSSL 1.1.1+, so this flag is the only component with the higher requirement. Enabling it verifies that libssl actually exports the QUIC API (some distros ship `no-quic` builds). HTTP/2 is always part of the build and needs no separate flag. See [HTTP/3](/en/http3) for details.

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
│   │   ├── server/                # → cwfr (main.c)
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
│   ├── app_init.c                 # The application module entry point
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
├── cwfr                           # Main executable
├── migrate                        # Migration utility
├── libapp.so                      # The application module (main.modules)
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

`libapp.so` stands apart: it is the application's own code — models, middleware, contexts — as a single shared library. The server loads it from the path in [`main.modules`](/en/config#modules) and calls `app_init()`; handlers resolve its symbols from that one instance. The core library, `libcwfr_framework.so`, holds nothing from the application, which is what makes it buildable once and reusable.

The handler and migration trees can be emitted outside the build directory:

```bash
cmake .. -DCWFR_HANDLER_OUT_DIR=$HOME/handlers \
         -DCWFR_MIGRATION_OUT_DIR=$HOME/migrations
```

## Installing

```bash
cmake --install build --prefix /opt/cwfr
```

```
/opt/cwfr/
├── bin/
│   ├── cwfr                       # the server
│   └── migrate                    # the migration utility
├── include/cwfr/                  # public headers, flat
└── lib/
    ├── cwfr/
    │   ├── libcwfr_framework.so   # plus .so.1 and .so.1.0.0
    │   ├── libapp.so
    │   ├── handlers/
    │   └── migrations/
    └── cmake/cwfr/                # the package for find_package(cwfr)
```

`cwfr` and `migrate` carry an `INSTALL_RPATH` of `$ORIGIN/../lib/cwfr`, so the tree relocates freely — neither `ldconfig` nor `LD_LIBRARY_PATH` is needed as long as `bin/` and `lib/cwfr/` keep their relative positions.

Headers are installed **flat**, into one directory: the core's sources include each other by bare name (`"httprequest.h"`, not `"protocols/http/httprequest.h"`), and flattening reproduces that with a single `-I` without publishing the core's internal layout.

`libcwfr_framework.so` carries a `SONAME` with its major version. Handlers record it, so an incompatible core upgrade becomes a clear load-time error instead of corruption at runtime.

Handler and migration install paths are overridable independently of the prefix — an absolute path ignores `--prefix`:

```bash
cmake .. -DCWFR_HANDLER_INSTALL_DIR=/srv/myapp/handlers \
         -DCWFR_MIGRATION_INSTALL_DIR=/srv/myapp/migrations
```

## Launch

```bash
# Run with a configuration file
./exec/cwfr -c ../config.json

# Stay in the foreground (what containers and supervisors need)
./exec/cwfr -c ../config.json -f
```

The application starts and listens on the ports defined in `config.json`. See [Configuration](./config.md) for details.

A `Release` build detaches from the terminal unless `-f` is given. `-f` is what you want wherever a supervisor watches the process: to it, a process that forks and exits looks like one that crashed.

**The exit status is meaningful in both modes.** The process does not report success until the configuration has been read, accepted and applied **and every worker is listening**, so `cwfr -c config.json && ...` behaves as written: a rejected configuration and a socket that cannot be bound both exit non-zero, and neither leaves a process behind. The detaching parent waits for that moment, which means the server is already accepting connections by the time the command returns.

## Hot reload

`SIGUSR1` reloads the configuration (`config.json`) without stopping the server:

```bash
pkill -USR1 cwfr
```
