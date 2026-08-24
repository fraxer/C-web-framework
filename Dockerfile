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
    libargon2-dev \
    libpq-dev \
    postgresql \
    libmysqlclient-dev \
    libhiredis-dev \
    libsqlite3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

COPY backend /build/backend

RUN cd backend && \
    mkdir -p build && \
    cd build && \
    cmake -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DINCLUDE_POSTGRESQL=yes \
        -DINCLUDE_MYSQL=yes \
        -DINCLUDE_REDIS=yes \
        -DINCLUDE_SQLITE=yes \
        .. && \
    ninja


# --- Stage 2: Runtime ---
FROM ubuntu:26.04 AS runtime

RUN apt-get update && apt-get install -y \
    libpcre2-8-0 \
    libssl3t64 \
    libxml2-16 \
    libidn2-0 \
    libunistring5 \
    libargon2-1 \
    libpq5 \
    libmariadb3 \
    libmysqlclient24 \
    libhiredis1.1.0 \
    libsqlite3-0 \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

RUN groupadd -r cwfr && useradd -r -g cwfr -s /sbin/nologin -c "cwfr user" cwfr

ENV CWFR_PREFIX=/opt/cwfr

COPY --from=builder /build/backend/build/exec ${CWFR_PREFIX}/bin
COPY --from=builder /build/backend/build/core/framework_shared/libcwfr_framework.so ${CWFR_PREFIX}/lib/cwfr/

RUN mkdir -p ${CWFR_PREFIX}/lib/cwfr/handlers ${CWFR_PREFIX}/lib/cwfr/migrations ${CWFR_PREFIX}/frontend

# Copy frontend files (for production)
COPY frontend/docs/.vitepress/dist ${CWFR_PREFIX}/frontend/

RUN chown -R cwfr:cwfr ${CWFR_PREFIX}

USER cwfr

ENV PATH="${CWFR_PREFIX}/bin:${PATH}"
ENV LD_LIBRARY_PATH="${CWFR_PREFIX}/lib/cwfr:${LD_LIBRARY_PATH}"

ENTRYPOINT ["cwfr"]
CMD ["-h"]
