/* Handlers for measuring per-connection handler concurrency
 * (docs/concurrency/00-handler-concurrency.md, §7).
 *
 * `delay` sleeps for a caller-controlled number of milliseconds and reports
 * when it started, when it finished and which thread ran it. Issuing N such
 * requests over one connection tells the two cases apart:
 *   serialized  -> the [start, end] intervals are disjoint, total ~= N*T
 *   concurrent  -> the intervals overlap,                  total ~= T
 * Times are CLOCK_MONOTONIC milliseconds, so they are directly comparable
 * across responses coming from the same machine.
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "http.h"
#include "json.h"

#define BENCH_DELAY_DEFAULT_MS 200
#define BENCH_DELAY_MAX_MS     10000

static long double __now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long double)ts.tv_sec * 1000.0L + (long double)ts.tv_nsec / 1000000.0L;
}

void delay(httpctx_t* ctx) {
    int ok = 0;
    int ms = query_param_int(ctx->request->query_, "ms", &ok);
    if (!ok)
        ms = BENCH_DELAY_DEFAULT_MS;

    if (ms < 0) ms = 0;
    if (ms > BENCH_DELAY_MAX_MS) ms = BENCH_DELAY_MAX_MS;

    const char* tag = query_param_char(ctx->request->query_, "tag", &ok);
    if (!ok)
        tag = "";

    const long double started = __now_ms();

    /* Deliberately a blocking sleep: it models an I/O-bound handler (db query,
     * http client, file read) without needing any of those to be configured. */
    if (ms > 0)
        usleep((useconds_t)ms * 1000);

    const long double finished = __now_ms();

    json_doc_t* doc = json_root_create_object();
    if (doc == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    json_token_t* object = json_root(doc);
    json_object_set(object, "tag", json_create_string(tag));
    json_object_set(object, "delay_ms", json_create_number(ms));
    json_object_set(object, "started_ms", json_create_number(started));
    json_object_set(object, "finished_ms", json_create_number(finished));
    json_object_set(object, "pid", json_create_number(getpid()));
    json_object_set(object, "tid", json_create_number(syscall(SYS_gettid)));

    const char* data = json_stringify(doc);
    if (data == NULL) {
        json_free(doc);
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->add_header(ctx->response, "Cache-Control", "no-store");
    ctx->response->send_data(ctx->response, data);

    json_free(doc);
}

/* Trailing fields (RFC 9113 §8.1) — the shape gRPC uses to report its status
 * after the body. docs/http2/08, phase E.1.
 *
 * `?body=N` sends N bytes before the trailers, so the same handler covers both
 * the "trailers after a real body" and the "trailers after nothing" cases.
 */
void trailers(httpctx_t* ctx) {
    int ok = 0;
    long body_size = query_param_int(ctx->request->query_, "body", &ok);
    if (!ok || body_size < 0) body_size = 0;
    if (body_size > 1024 * 1024) body_size = 1024 * 1024;

    ctx->response->add_trailer(ctx->response, "grpc-status", "0");
    ctx->response->add_trailer(ctx->response, "grpc-message", "ok");

    /* Echo back a trailing field the *client* sent, which is the only way to
     * show from outside that request trailers reach a handler at all
     * (docs/http2/10, T.1). "none" when there was none, so the probe can tell
     * "not received" from "not implemented". */
    const http_header_t* echo = ctx->request->get_trailer(ctx->request, "x-checksum");
    ctx->response->add_trailer(ctx->response, "x-echo",
                               echo != NULL ? echo->value : "none");

    if (body_size == 0) {
        ctx->response->send_datan(ctx->response, "", 0);
        return;
    }

    char* body = malloc((size_t)body_size);
    if (body == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    memset(body, 'x', (size_t)body_size);
    ctx->response->send_datan(ctx->response, body, (size_t)body_size);
    free(body);
}

/* 103 Early Hints (RFC 8297) — docs/http2/08, phase E.2.
 *
 * Sends the hints, then keeps working for `?ms=` milliseconds before the real
 * response. That gap is the whole point of the feature: the client can start
 * fetching the preloads while the handler is still busy, which is only
 * observable if the 103 really leaves the server before the 200.
 */
void early_hints(httpctx_t* ctx) {
    int ok = 0;
    int ms = query_param_int(ctx->request->query_, "ms", &ok);
    if (!ok || ms < 0) ms = 100;
    if (ms > 5000) ms = 5000;

    ctx->response->add_early_hint(ctx->response, "link", "</style.css>; rel=preload; as=style");
    ctx->response->add_early_hint(ctx->response, "link", "</app.js>; rel=preload; as=script");
    ctx->response->send_early_hints(ctx->response);

    usleep((useconds_t)ms * 1000);

    ctx->response->send_data(ctx->response, "final");
}
