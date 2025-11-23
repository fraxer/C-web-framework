#include <linux/limits.h>

#include "http.h"
#include "json.h"
#include "helpers.h"
#include "httpclient.h"
#include "storage.h"

void client_get(httpctx_t* ctx) {
    const char* url = "http://example.com";
    const int timeout = 3;
    httpclient_t* client = httpclient_init(ROUTE_GET, url, timeout);
    if (client == NULL) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(500));
        return;
    }

    httprequest_t* req = client->request;
    req->add_header(req, "Accept-Encoding", "gzip");
    req->add_header(req, "User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36");

    httpresponse_t* res = client->send(client);
    if (!res) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        client->free(client);
        return;
    }
    if (res->status_code != 200) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(res->status_code));
        client->free(client);
        return;
    }

    char* document = res->get_payload(res);
    if (!document) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "response error");
        client->free(client);
        return;
    }

    ctx->response->send_data(ctx->response, document);

    free(document);
    client->free(client);
}

void client_post_urlencoded(httpctx_t* ctx) {
    const char* url = "http://example.com/post_urlencoded";
    const int timeout = 3;
    httpclient_t* client = httpclient_init(ROUTE_POST, url, timeout);
    if (client == NULL) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(500));
        return;
    }

    httprequest_t* req = client->request;
    req->append_urlencoded(req, "key1", "value1");
    req->append_urlencoded(req, "key2", "value2");

    httpresponse_t* res = client->send(client);
    if (!res) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        client->free(client);
        return;
    }
    if (res->status_code != 200) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(res->status_code));
        client->free(client);
        return;
    }

    char* document = res->get_payload(res);
    if (!document) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "response error");
        client->free(client);
        return;
    }

    ctx->response->send_data(ctx->response, document);

    free(document);
    client->free(client);
}

void client_post_formdata(httpctx_t* ctx) {
    const char* url = "http://example.com/post_formdata";
    const int timeout = 3;
    httpclient_t* client = httpclient_init(ROUTE_POST, url, timeout);
    if (client == NULL) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(500));
        return;
    }

    httprequest_t* req = client->request;
    req->append_formdata_raw(req, "key", "raw", "text/plain");
    req->append_formdata_text(req, "key1", "text");

    json_doc_t* doc = json_root_create_object();
    json_token_t* object = json_root(doc);
    json_object_set(object, "bool", json_create_bool(1));
    json_object_set(object, "int", json_create_number(12));
    req->append_formdata_json(req, "key2", doc);
    json_free(doc);

    file_t file = storage_file_get("server", "/file.txt");
    if (file.ok) {
        req->append_formdata_file(req, "key", &file);
        file.close(&file);
    }

    httpresponse_t* res = client->send(client);
    if (!res) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        client->free(client);
        return;
    }
    if (res->status_code != 200) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(res->status_code));
        client->free(client);
        return;
    }

    char* document = res->get_payload(res);
    if (!document) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "response error");
        client->free(client);
        return;
    }

    ctx->response->send_data(ctx->response, document);

    free(document);
    client->free(client);
}

void client_post_raw_payload(httpctx_t* ctx) {
    const char* url = "http://example.com/post";
    const int timeout = 3;
    httpclient_t* client = httpclient_init(ROUTE_POST, url, timeout);
    if (client == NULL) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(500));
        return;
    }

    httprequest_t* req = client->request;

    json_doc_t* doc = json_root_create_object();
    json_token_t* object = json_root(doc);
    json_object_set(object, "bool", json_create_bool(1));
    json_object_set(object, "int", json_create_number(12));
    req->set_payload_json(req, doc);
    json_free(doc);

    httpresponse_t* res = client->send(client);
    if (!res) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        client->free(client);
        return;
    }
    if (res->status_code != 200) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(res->status_code));
        client->free(client);
        return;
    }

    char* document = res->get_payload(res);
    if (!document) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "response error");
        client->free(client);
        return;
    }

    ctx->response->send_data(ctx->response, document);

    free(document);
    client->free(client);

}
