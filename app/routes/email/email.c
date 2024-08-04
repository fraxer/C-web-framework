#include "http1.h"
#include "mail.h"

void mail_send(httpctx_t* ctx) {
    mail_payload_t payload = {
        .from = "noreply@cwebframework.tech",
        .from_name = "Alexander Korchagin",
        .to = "mail@example.com",
        .subject = "Test mail",
        .body = "Just text"
    };
    if (!send_mail(&payload)) {
        ctx->response->data(ctx->response, "Error send mail");
        return;
    }

    ctx->response->data(ctx->response, "done");
}