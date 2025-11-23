---
outline: deep
description: Building C Web Framework from source code. Release and Debug build modes, CMake configuration, project structure and application launch.
---

# Build and run

## Quick start

```bash
# Clone the repository
git clone git@github.com:fraxer/C-web-framework.git
cd C-web-framework

# Create build directory
mkdir build && cd build

# Configure project with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DINCLUDE_POSTGRESQL=yes \
         -DINCLUDE_MYSQL=yes \
         -DINCLUDE_REDIS=yes

# Build the project
cmake --build . -j4

# Run the application
./exec/cpdy -c /path/to/config.json
```

## Build modes

| Mode | Description |
|------|-------------|
| `Release` | Performance optimization, no debug information. Recommended for production |
| `Debug` | Debug information enabled, AddressSanitizer for memory error detection |
| `RelWithDebInfo` | Performance optimization with debug information |
| `MinSizeRel` | Binary size optimization |

```bash
# Build in Debug mode
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DINCLUDE_POSTGRESQL=yes \
         -DINCLUDE_MYSQL=yes \
         -DINCLUDE_REDIS=yes
```

## CMake parameters

### Databases

Enable database support:

```bash
-DINCLUDE_POSTGRESQL=yes  # PostgreSQL support
-DINCLUDE_MYSQL=yes       # MySQL support
-DINCLUDE_REDIS=yes       # Redis support
```

### Compiler

Specify a specific compiler version if necessary:

```bash
cmake .. -DCMAKE_C_COMPILER=/usr/bin/gcc-12
```

## Project structure

```
project/
├── core/                          # Framework core
│   ├── framework/                 # Framework components
│   │   ├── database/             # Database (PostgreSQL, MySQL, Redis)
│   │   ├── model/                # ORM model system
│   │   ├── session/              # Session system
│   │   ├── storage/              # File storage (FS, S3)
│   │   ├── view/                 # Template engine
│   │   ├── middleware/           # Middleware system
│   │   └── query/                # Query builder
│   ├── protocols/                # Protocol implementations
│   │   ├── http/                 # HTTP/1.1 server and client
│   │   ├── websocket/            # WebSocket server
│   │   └── smtp/                 # SMTP client, DKIM
│   ├── src/                      # Core components
│   │   ├── broadcast/            # Broadcasting system
│   │   ├── connection/           # Connection management
│   │   ├── server/               # HTTP server
│   │   ├── route/                # Routing system
│   │   ├── thread/               # Multithreading
│   │   └── multiplexing/         # Epoll multiplexing
│   └── misc/                     # Utilities
│       ├── str.h                 # Dynamic strings
│       ├── array.h               # Arrays
│       ├── hashmap.h/map.h       # Associative arrays
│       ├── json.h                # JSON parser
│       ├── log.h                 # Logging
│       └── gzip.h                # Gzip compression
│
└── app/                          # User application
    ├── routes/                   # HTTP and WebSocket handlers
    │   ├── auth/                # Authentication (login, registration)
    │   ├── index/               # Main page
    │   ├── ws/                  # WebSocket handlers
    │   ├── files/               # File operations
    │   ├── models/              # API model operations
    │   ├── email/               # Email sending
    │   └── db/                  # Database examples
    ├── models/                   # Data models
    │   ├── user.c               # User model
    │   ├── role.c               # Role model
    │   ├── permission.c         # Permission model
    │   └── *view.c              # View models for JOIN queries
    ├── middlewares/              # Custom middlewares
    │   ├── httpmiddlewares.c    # HTTP middleware (auth, rate limit)
    │   └── wsmiddlewares.c      # WebSocket middleware
    ├── migrations/               # Database migrations
    │   ├── s1/                  # Migrations for server s1
    │   └── s2/                  # Migrations for server s2
    ├── broadcasting/             # Broadcasting channels
    │   └── mybroadcast.c        # Broadcasting channel example
    ├── auth/                     # Authentication module
    │   ├── auth.c               # Authentication functions
    │   ├── password_validator.c # Password validation
    │   └── email_validator.c    # Email validation
    ├── contexts/                 # Request contexts
    │   ├── httpcontext.c        # HTTP context
    │   └── wscontext.c          # WebSocket context
    └── views/                    # Templates
        ├── index.tpl            # Main page
        └── header.tpl           # Header

config.json                       # Application configuration
```

## Build results

After building, files are located in the `build/exec` directory:

```
build/exec/
├── cpdy                          # Main executable
├── migrate                       # Migration utility
├── handlers/                     # Compiled handlers
│   ├── libindexpage.so
│   ├── libwsindexpage.so
│   └── ...
└── migrations/                   # Compiled migrations
    ├── s1/
    │   └── lib2023-04-04_17-55-00_create_users_table.so
    └── ...
```

## Launch

```bash
# Run with configuration file
./build/exec/cpdy -c /path/to/config.json
```

The application will start and listen on the ports specified in the configuration.

## Hot reload

To apply configuration changes without stopping the server:

```bash
pkill -USR1 cpdy
```
