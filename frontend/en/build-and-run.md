---
outline: deep
description: Building a project from source code, location of handlers, project structure, application launch
---

# Build and run

```bash
# Cloning the repository and moving to the project directory
git clone git@github.com:fraxer/server.git;
cd server;
# Setting up the project and creating the build system
cmake -DINCLUDE_POSTGRESQL=yes -DINCLUDE_MYSQL=yes -DINCLUDE_REDIS=yes ..;
# Calling the build system to compile/link the project
cmake --build .;
# Application launch
exec/cpdy -c /var/www/server/config.json;
```

## Build modes

* `Debug` - optimization disabled, debug information enabled
* `Release` - optimization of execution speed, debug information excluded
* `MinSizeRel` - binary file size optimization, debug information excluded
* `RelWithDebInfo` - runtime optimization, debug information enabled

```bash
cmake -DCMAKE_BUILD_TYPE:STRING=Debug -DINCLUDE_REDIS=yes ..;
```

## Compiler

Specify specific compiler version if necessary

```bash
cmake -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/gcc-12 ..;
```

## Handlers

Handlers are located in the directory `handlers`.

```
handlers/
    CMakeLists.txt
    indexpage.c
    wsindexpage.c
```

## Build

The compiled files are located in the directory `build/exec`.

```
handlers/
    libindexpage.so
    libwsindexpage.so
    ...
migrations/
    s1/
        lib2023-04-04_17-55-00_create_users_table.so
        ...
    ...
cpdy
migrate
```
