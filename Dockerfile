# cwfr - High-performance event-driven C web framework
# Multi-stage Dockerfile for production builds

# --- Stage 0: Toolchain ---
# The compiler and the -dev packages, split out of the build stage so the SDK
# stage below can reuse them without repeating the package list: an application
# that builds handlers against the installed framework needs exactly the same
# headers the framework itself was compiled with.
FROM ubuntu:26.04 AS toolchain

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    libpcre2-dev \
    zlib1g-dev \
    libssl-dev \
    libxml2-dev \
    libidn2-dev \
    libunistring-dev \
    libpq-dev \
    postgresql \
    libmysqlclient-dev \
    libhiredis-dev \
    libsqlite3-dev \
    && rm -rf /var/lib/apt/lists/*


# --- Stage 1: Build ---
FROM toolchain AS builder

WORKDIR /build

COPY backend /build/backend

# Build and install into the prefix the runtime stage will use verbatim, rather
# than copying individual files out of the build tree: `cmake --install` is what
# lays out bin/ and lib/cwfr/ correctly, applies the $ORIGIN/../lib/cwfr RPATH,
# and creates the libcwfr_framework.so.<major> SONAME links that handler modules
# are linked against.
RUN cmake -G Ninja -S backend -B backend/build \
        -DCMAKE_BUILD_TYPE=Release \
        -DINCLUDE_POSTGRESQL=yes \
        -DINCLUDE_MYSQL=yes \
        -DINCLUDE_REDIS=yes \
        -DINCLUDE_SQLITE=yes \
        -DINCLUDE_HTTP3=yes && \
    cmake --build backend/build && \
    cmake --install backend/build --prefix /opt/cwfr


# --- Stage 1b: SDK ---
# What the runtime stage deliberately throws away: the public headers and the
# exported CMake package, on top of the toolchain that compiles against them.
#
# This is the image an application builds its handlers in. It carries the whole
# install prefix rather than the framework sources, which is the arrangement
# backend/app/CMakeLists.txt describes -- `find_package(cwfr)` hands over
# cwfr::framework plus cwfr_add_handlers()/cwfr_add_migrations(), and the app is
# built and rebuilt without the core present at all:
#
#   docker build -t cwfr-sdk:1.0.0 --target sdk .
#   FROM cwfr-sdk:1.0.0 AS builder
#   RUN cmake -S app -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
#
# cwfr_DIR is preset, so the application's CMake invocation needs no -D for it.
FROM toolchain AS sdk

ENV CWFR_PREFIX=/opt/cwfr
ENV cwfr_DIR=/opt/cwfr/lib/cmake/cwfr
ENV PATH="/opt/cwfr/bin:${PATH}"
ENV LD_LIBRARY_PATH="/opt/cwfr/lib/cwfr:${LD_LIBRARY_PATH}"

COPY --from=builder /opt/cwfr /opt/cwfr

# Без примера приложения: та же сборка ставит в этот префикс и libapp.so с
# обработчиками и миграциями демонстрационного приложения. В SDK им не место —
# иначе `migrate up all` в постороннем проекте завёл бы у него таблицы user,
# role и permission из примера.
RUN rm -rf /opt/cwfr/lib/cwfr/libapp.so \
           /opt/cwfr/lib/cwfr/handlers \
           /opt/cwfr/lib/cwfr/migrations


# --- Stage 2: Frontend (VitePress documentation site) ---
# npm >= 11 is required: the lock file's deduped @types/node tree only
# validates with it (older npm resolves a "*" spec differently and fails).
FROM node:24-alpine AS frontend

WORKDIR /site

COPY frontend/package.json frontend/package-lock.json ./
RUN npm ci

COPY frontend/docs ./docs
RUN npm run docs:build


# --- Stage 3: Runtime base ---
# The server and its shared libraries, with no documentation site attached.
#
# Split from `runtime` so that an application image can start FROM here and add
# only its own handlers -- otherwise every such image would have to repeat this
# package list, and building it would pull in the Node stage it has no use for:
#
#   docker build -t cwfr-runtime:1.0.0 --target runtime-base .
FROM ubuntu:26.04 AS runtime-base

RUN apt-get update && apt-get install -y \
    libpcre2-8-0 \
    libssl3t64 \
    libxml2-16 \
    libidn2-0 \
    libunistring5 \
    libpq5 \
    libmariadb3 \
    libmysqlclient24 \
    libhiredis1.1.0 \
    libsqlite3-0 \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

RUN groupadd -r cwfr && useradd -r -g cwfr -s /sbin/nologin -c "cwfr user" cwfr

ENV CWFR_PREFIX=/opt/cwfr

# Only what the server needs to run: the headers and the CMake package that
# `cmake --install` also produced belong in an SDK image, not this one.
COPY --from=builder /opt/cwfr/bin ${CWFR_PREFIX}/bin
COPY --from=builder /opt/cwfr/lib/cwfr ${CWFR_PREFIX}/lib/cwfr

RUN mkdir -p ${CWFR_PREFIX}/lib/cwfr/handlers ${CWFR_PREFIX}/lib/cwfr/migrations ${CWFR_PREFIX}/frontend

RUN chown -R cwfr:cwfr ${CWFR_PREFIX}

USER cwfr

ENV PATH="${CWFR_PREFIX}/bin:${PATH}"
ENV LD_LIBRARY_PATH="${CWFR_PREFIX}/lib/cwfr:${LD_LIBRARY_PATH}"

ENTRYPOINT ["cwfr"]
CMD ["-h"]


# --- Stage 4: Runtime ---
# runtime-base plus the VitePress documentation site this repository serves.
FROM runtime-base AS runtime

USER root
COPY --from=frontend /site/docs/.vitepress/dist ${CWFR_PREFIX}/frontend/
RUN chown -R cwfr:cwfr ${CWFR_PREFIX}/frontend
USER cwfr
