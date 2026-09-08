#include "http.h"
#include "log.h"
#include "mail.h"

void mail_send(httpctx_t* ctx) {
    mail_payload_t payload = {
        .from = "noreply@cwebframework.tech",
        .from_name = "Alexander",
        .to = "mail@example.com",
        .subject = "Test mail",
        .body = "Just text"
    };
    /* send_mail() alone is enough when the reason does not matter; asking for a
     * mail_result_t is what lets a 4xx ("try later") be told from a 5xx. */
    mail_result_t result;
    if (!send_mail_result(&payload, &result)) {
        log_error("[mail_send] %d %s\n", result.status, result.error);

        ctx->response->status_code = result.status >= 400 && result.status < 500 ? 503 : 500;
        ctx->response->send_data(ctx->response, "Error send mail");
        return;
    }

    ctx->response->send_data(ctx->response, "done");
}