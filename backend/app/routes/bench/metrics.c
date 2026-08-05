/* Reads out the concurrency counters described in
 * docs/concurrency/00-handler-concurrency.md, phase D.
 *
 * The counters live in the framework (core/src/metrics); this handler is only a
 * window onto them, so an application that wants them elsewhere — a different
 * path, an admin vhost, a push to a collector — writes its own handler around
 * metrics_snapshot_json() instead of adopting this one.
 *
 *   GET /metrics            snapshot
 *   GET /metrics?reset=1    snapshot, then zero the counters
 *
 * The reset is what makes a before/after comparison honest: hit it once before
 * the run, once after, and everything reported belongs to the run.
 */

#include "http.h"
#include "json.h"
#include "metrics.h"

void get(httpctx_t* ctx) {
    int ok = 0;
    const int reset = query_param_int(ctx->request->query_, "reset", &ok);

    json_doc_t* doc = metrics_snapshot_json();
    if (doc == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    /* Reset after the snapshot, so the response still describes the window that
     * just ended rather than the empty one that follows it. */
    if (ok && reset)
        metrics_reset();

    ctx->response->send_json(ctx->response, doc);

    json_free(doc);
}
