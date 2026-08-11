#!/bin/bash
#
# quic-interop-runner endpoint entry point (docs/http3/08 §3.3e).
#
# The runner hands everything in through the environment and two mounts: /www
# holds the files to serve, /certs the key and chain it generated. The contract
# is small and unusual in one place -- exit code 127 means "this endpoint does
# not implement this test case", and the runner reports it as unsupported
# rather than failed. Anything else non-zero is a failure.

set -e

# Routing into the network simulator. Provided by the base image; without it the
# endpoint talks to the docker bridge instead of through ns3, and every packet
# arrives with the checksum offloaded and therefore invalid.
/setup.sh

if [ "$ROLE" != "server" ]; then
    # Client side is not implemented: quicclient exists for our own testing but
    # speaks nothing the runner's REQUESTS format expects, and a half-working
    # client would report failures against every other implementation rather
    # than against us.
    exit 127
fi

case "$TESTCASE" in
    # http3 is the one case the runner drives over h3; the rest move files with
    # HTTP/0.9 under ALPN hq-interop, which this image is built to speak
    # (INCLUDE_HQ_INTEROP, docs/http3/08 §3e).
    http3|handshake|transfer|longrtt|chacha20|multiplexing|retry|resumption|\
    multiconnect|blackhole|handshakeloss|transferloss|handshakecorruption|\
    transfercorruption|rebind-port|rebind-addr|amplificationlimit|goodput|\
    crosstraffic) ;;

    # Declined on purpose, each for a reason that is a real gap rather than a
    # configuration choice:
    #   ipv6                 -- the socket layer is AF_INET only (docs/http3/01)
    #   zerortt              -- 0-RTT is phase 9; early data is refused outright
    #   v2                   -- QUIC v2 is phase 9
    #   connectionmigration  -- needs a server preferred_address, which we never
    #                           offer (the transport parameter is parsed and
    #                           deliberately not produced)
    #   ecn, keyupdate       -- ECN is unimplemented; keyupdate is client-driven
    *) exit 127 ;;
esac

CONFIG=/tmp/interop.json

# What the test case asks of the server itself, rather than of the transfer.
#
# `retry` fails unless a Retry is actually sent, and `handshake` fails if one is;
# the default `auto` policy sends none until a thousand handshakes are in flight,
# which is right in production and useless as an answer to either case.
# `chacha20` requires that ChaCha20 be the *only* ciphersuite on offer.
RETRY=auto
CIPHERS="TLS_AES_128_GCM_SHA256 TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256"

case "$TESTCASE" in
    retry)     RETRY=always ;;
    handshake) RETRY=never ;;
    chacha20)  CIPHERS="TLS_CHACHA20_POLY1305_SHA256" ;;
esac

# Counters are on, and `/metrics` is served over the TCP listener (docs/http3/08
# §3n). It is the only way to ask this endpoint what it did with datagrams it
# never turned into a connection: the runner keeps no log mount, and a drop that
# happens before `accepted` leaves no trace anywhere else. Read it from inside
# the container while the run is still up:
#
#   docker exec sim-server curl -sk https://localhost/metrics
#
# Over TCP on purpose -- asking QUIC about its own drops through QUIC would go
# missing exactly when the answer matters. The path is ours, not the runner's:
# every case fetches generated file names out of /www, so nothing can collide.
#
# One vhost, every name the runner may address it by. The compose file gives the
# server 193.167.100.100 and the hosts entries server4/server6/server46; which
# one appears in :authority depends on the test case, and a name we do not know
# would be routed to the listener's first vhost -- here, harmlessly, the same
# one, but the list is written out so that stays true if a second is ever added.
cat > "$CONFIG" <<EOF
{
    "main": {
        "workers": 1,
        "threads": 4,
        "reload": "hard",
        "buffer_size": 16384,
        "client_max_body_size": 1073741824,
        "tmp": "/tmp",
        "gzip": ["text/html"],
        "log": { "enabled": true, "level": "info" },
        "env": {
            "http3_so_rcvbuf": 4194304,
            "http3_max_connections": 1024,
            "http3_retry": "$RETRY",
            "metrics": true
        }
    },
    "servers": {
        "s1": {
            "domains": ["server", "server4", "server6", "server46",
                        "193.167.100.100"],
            "ip": "0.0.0.0",
            "port": 443,
            "root": "/www",
            "tls": {
                "fullchain": "/certs/cert.pem",
                "private": "/certs/priv.key",
                "ciphers": "$CIPHERS"
            },
            "http3": { "enabled": true, "port": 443 },
            "http": {
                "routes": {
                    "/metrics": {
                        "GET": {
                            "file": "/cwfr/handlers/bench/lib_metrics.so",
                            "function": "get"
                        }
                    }
                }
            }
        }
    },
    "mimetypes": {
        "application/octet-stream": ["bin"],
        "text/html": ["html"]
    }
}
EOF

echo "cwfr interop endpoint: TESTCASE=$TESTCASE"

# -f, and it is not optional here: a Release build daemonises, and a container
# whose entry point forks and exits is a container that died on start-up. The
# runner then waits out its timeout against a server that is, in fact, running.
exec /cwfr/exec/cwfr -c "$CONFIG" -f
