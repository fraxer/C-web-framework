#include "http1.h"
#include "mail.h"

void mail_send(__attribute__((unused))http1request_t* request, http1response_t* response) {
    mail_payload_t payload = {
        .from = "noreply@cwebframework.tech",
        .from_name = "Alexander Korchagin",
        .to = "mail@example.com",
        .subject = "Test mail",
        .body = "Just text"
    };
    if (!send_mail(&payload)) {
        response->data(response, "Error send mail");
        return;
    }

    response->data(response, "done");
}