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

FROM ubuntu:26.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build pkg-config curl \
        libpcre2-dev libssl-dev zlib1g-dev libidn2-dev libxml2-dev libunistring-dev \
    && rm -rf /var/lib/apt/lists/*

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

RUN apt-get update && apt-get install -y --no-install-recommends \
        libpcre2-8-0 libssl3t64 zlib1g libidn2-0 libxml2-16 libunistring5 curl \
        ethtool net-tools iproute2 ca-certificates \
    && rm -rf /var/lib/apt/lists/*

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
