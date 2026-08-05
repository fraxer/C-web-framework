/* WebSocket counterpart of bench.c's `delay`, for phase C of
 * docs/concurrency/00-handler-concurrency.md.
 *
 * Sends N messages down ONE websocket connection and reports, per reply, when
 * the handler started, when it finished and which thread ran it. Serialized
 * handlers give disjoint [start, finish] intervals and a total of ~N*T;
 * concurrent ones overlap and total ~T. Times are CLOCK_MONOTONIC ms, directly
 * comparable across replies from the same machine.
 *
 * Message form (resource protocol): "GET /wsdelay?ms=200&tag=a".
 */

#define _GNU_SOURCE

#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "websockets.h"
#include "json.h"

#define WSBENCH_DELAY_DEFAULT_MS 200
#define WSBENCH_DELAY_MAX_MS     10000

static long double __now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long double)ts.tv_sec * 1000.0L + (long double)ts.tv_nsec / 1000000.0L;
}

void delay(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;

    int ok = 0;
    int ms = query_param_int(protocol->query_, "ms", &ok);
    if (!ok)
        ms = WSBENCH_DELAY_DEFAULT_MS;

    if (ms < 0) ms = 0;
    if (ms > WSBENCH_DELAY_MAX_MS) ms = WSBENCH_DELAY_MAX_MS;

    const char* tag = protocol->get_query(protocol, "tag", &ok);
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
        ctx->response->send_text(ctx->response, "{\"error\":\"out of memory\"}");
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
        ctx->response->send_text(ctx->response, "{\"error\":\"stringify failed\"}");
        return;
    }

    ctx->response->send_text(ctx->response, data);

    json_free(doc);
}
