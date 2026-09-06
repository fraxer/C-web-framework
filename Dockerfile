# cwfr - High-performance event-driven C web framework
# Multi-stage Dockerfile for production builds

# --- Stage 1: Build ---
FROM ubuntu:26.04 AS builder

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
        -DINCLUDE_SQLITE=yes && \
    cmake --build backend/build && \
    cmake --install backend/build --prefix /opt/cwfr


# --- Stage 2: Frontend (VitePress documentation site) ---
# npm >= 11 is required: the lock file's deduped @types/node tree only
# validates with it (older npm resolves a "*" spec differently and fails).
FROM node:24-alpine AS frontend

WORKDIR /site

COPY frontend/package.json frontend/package-lock.json ./
RUN npm ci

COPY frontend/docs ./docs
RUN npm run docs:build


# --- Stage 3: Runtime ---
FROM ubuntu:26.04 AS runtime

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

# Copy frontend files (built in the frontend stage)
COPY --from=frontend /site/docs/.vitepress/dist ${CWFR_PREFIX}/frontend/

RUN chown -R cwfr:cwfr ${CWFR_PREFIX}

USER cwfr

ENV PATH="${CWFR_PREFIX}/bin:${PATH}"
ENV LD_LIBRARY_PATH="${CWFR_PREFIX}/lib/cwfr:${LD_LIBRARY_PATH}"

ENTRYPOINT ["cwfr"]
CMD ["-h"]
