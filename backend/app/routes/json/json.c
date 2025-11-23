#include "http.h"
#include "json.h"

void get(httpctx_t* ctx) {
    json_doc_t* doc2 = json_parse("[\"key\", 5, true, \"value\", \"Hello 😅\", {\"key\": \"value\", \"number\": 2}]");
    doc2->ascii_mode = 1;
    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(doc2));
    json_free(doc2);
}
