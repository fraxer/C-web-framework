# cwfr as a quic-interop-runner server endpoint (core/docs/http3/08 §3e).
#
# Build from backend/:
#   docker build -f interop/Dockerfile -t cwfr-interop .
#
# It lives here rather than in core/ because core is not a standalone CMake
# project: find_package(OpenSSL) is done by backend/CMakeLists.txt, which then
# adds core as a subdirectory. The build context has to contain both.
#
# Two more things force the shape of this file:
#
#  - HTTP/3 needs the OpenSSL QUIC TLS API, which landed in 3.5. The runner's
#    endpoint base image is Ubuntu 24.04 and carries OpenSSL 3.0, so it cannot
#    be the runtime. Both stages are Ubuntu 26.04 instead, and the one thing
#    actually needed from the base image -- /setup.sh, which routes the endpoint
#    through the network simulator -- is copied in.
#  - The databases are switched off. They pull in a dozen client libraries that
#    an interop endpoint never touches, and every one of them is another way for
#    the image build to break for reasons unrelated to QUIC.
#
# And one wart that is not this file's fault: framework_shared links
# `-l:libpcre.so.3`, which is PCRE1 -- end of life since 2020 and no longer
# packaged by Ubuntu 26.04. The .deb is fetched from the 24.04 pool to keep the
# image buildable. The real fix is a move to PCRE2 and does not belong here.

ARG PCRE1_POOL=http://archive.ubuntu.com/ubuntu/pool/main/p/pcre3
ARG PCRE1_VER=8.39-13ubuntu0.22.04.1

FROM ubuntu:26.04 AS builder
ARG PCRE1_POOL
ARG PCRE1_VER

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build pkg-config curl \
        libssl-dev zlib1g-dev libidn2-dev libxml2-dev libunistring-dev libargon2-dev \
    && curl -fsSLO "$PCRE1_POOL/libpcre3_${PCRE1_VER}_amd64.deb" \
    && curl -fsSLO "$PCRE1_POOL/libpcre3-dev_${PCRE1_VER}_amd64.deb" \
    && dpkg -i "libpcre3_${PCRE1_VER}_amd64.deb" \
    # Unpacked rather than installed: the -dev package depends on the 16- and
    # 32-bit variants this build never touches, and only its headers and the
    # .so symlink are wanted.
    && dpkg-deb -x "libpcre3-dev_${PCRE1_VER}_amd64.deb" / \
    && rm -f ./*.deb && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . /src

RUN cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DINCLUDE_HTTP3=yes \
        -DINCLUDE_HQ_INTEROP=yes \
        -DINCLUDE_POSTGRESQL=no \
        -DINCLUDE_MYSQL=no \
        -DINCLUDE_REDIS=no \
        -DINCLUDE_SQLITE=no \
    && cmake --build build -j"$(nproc)"

FROM ubuntu:26.04
ARG PCRE1_POOL
ARG PCRE1_VER

RUN apt-get update && apt-get install -y --no-install-recommends \
        libssl3t64 zlib1g libidn2-0 libxml2-16 libunistring5 libargon2-1 curl \
        ethtool net-tools iproute2 ca-certificates \
    && curl -fsSLO "$PCRE1_POOL/libpcre3_${PCRE1_VER}_amd64.deb" \
    && dpkg -i "libpcre3_${PCRE1_VER}_amd64.deb" \
    && rm -f ./*.deb && rm -rf /var/lib/apt/lists/*

# The simulator routing script, and only that: the rest of the base image is a
# 24.04 userland this binary cannot run against.
COPY --from=martenseemann/quic-network-simulator-endpoint:latest /setup.sh /setup.sh

COPY --from=builder /src/build/exec/cwfr /cwfr/exec/cwfr
COPY --from=builder /src/build/core/framework_shared/libcwfr_framework.so \
                    /cwfr/lib/libcwfr_framework.so

# The one handler this endpoint loads: a window onto the counters, routed at
# /metrics by run_endpoint.sh. Everything else the runner asks for is a static
# file out of /www.
COPY --from=builder /src/build/exec/handlers/bench/lib_metrics.so \
                    /cwfr/handlers/bench/lib_metrics.so

# The framework is one shared library, found by rpath from the build tree --
# which does not survive the copy.
ENV LD_LIBRARY_PATH=/cwfr/lib

COPY interop/run_endpoint.sh /run_endpoint.sh
RUN chmod +x /run_endpoint.sh /setup.sh

ENTRYPOINT ["/run_endpoint.sh"]
