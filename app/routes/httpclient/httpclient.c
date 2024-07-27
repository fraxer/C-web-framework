#include <linux/limits.h>

#include "http1.h"
#include "json.h"
#include "helpers.h"
#include "httpclient.h"
#include "storage.h"

void client_get(http1request_t* request, http1response_t* response) {
    (void)request;

    const char* url = "http://example.com";
    const int timeout = 3;
    httpclient_t* client = httpclient_init(ROUTE_GET, url, timeout);
    if (client == NULL) {
        response->data(response, http1response_status_string(500));
        return;
    }

    http1request_t* req = client->request;
    req->header_add(req, "Accept-Encoding", "gzip");
    req->header_add(req, "User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36");

    http1response_t* res = client->send(client);
    if (!res) {
        response->status_code = 500;
        response->data(response, "error");
        client->free(client);
        return;
    }
    if (res->status_code != 200) {
        response->data(response, http1response_status_string(res->status_code));
        client->free(client);
        return;
    }

    char* document = res->payload(res);
    if (!document) {
        response->status_code = 500;
        response->data(response, "response error");
        client->free(client);
        return;
    }

    response->data(response, document);

    free(document);
    client->free(client);
}

void client_post_urlencoded(http1request_t* request, http1response_t* response) {
    (void)request;

    const char* url = "http://example.com/post_urlencoded";
    const int timeout = 3;
    httpclient_t* client = httpclient_init(ROUTE_POST, url, timeout);
    if (client == NULL) {
        response->data(response, http1response_status_string(500));
        return;
    }

    http1request_t* req = client->request;
    req->append_urlencoded(req, "key1", "value1");
    req->append_urlencoded(req, "key2", "value2");

    http1response_t* res = client->send(client);
    if (!res) {
        response->status_code = 500;
        response->data(response, "error");
        client->free(client);
        return;
    }
    if (res->status_code != 200) {
        response->data(response, http1response_status_string(res->status_code));
        client->free(client);
        return;
    }

    char* document = res->payload(res);
    if (!document) {
        response->status_code = 500;
        response->data(response, "response error");
        client->free(client);
        return;
    }

    response->data(response, document);

    free(document);
    client->free(client);
}

void client_post_formdata(http1request_t* request, http1response_t* response) {
    (void)request;

    const char* url = "http://example.com/post_formdata";
    const int timeout = 3;
    httpclient_t* client = httpclient_init(ROUTE_POST, url, timeout);
    if (client == NULL) {
        response->data(response, http1response_status_string(500));
        return;
    }

    http1request_t* req = client->request;
    req->append_formdata_raw(req, "key", "raw", "text/plain");
    req->append_formdata_text(req, "key1", "text");

    jsondoc_t* doc = json_init();
    jsontok_t* object = json_create_object(doc);
    json_object_set(object, "bool", json_create_bool(doc, 1));
    json_object_set(object, "int", json_create_int(doc, 12));
    req->append_formdata_json(req, "key2", doc);
    json_free(doc);

    file_t file = storage_file_get("server", "/file.txt");
    if (file.ok) {
        req->append_formdata_file(req, "key", &file);
        file.close(&file);
    }

    http1response_t* res = client->send(client);
    if (!res) {
        response->status_code = 500;
        response->data(response, "error");
        client->free(client);
        return;
    }
    if (res->status_code != 200) {
        response->data(response, http1response_status_string(res->status_code));
        client->free(client);
        return;
    }

    char* document = res->payload(res);
    if (!document) {
        response->status_code = 500;
        response->data(response, "response error");
        client->free(client);
        return;
    }

    response->data(response, document);

    free(document);
    client->free(client);
}

void client_post_raw_payload(http1request_t* request, http1response_t* response) {
    (void)request;

    const char* url = "http://example.com/post";
    const int timeout = 3;
    httpclient_t* client = httpclient_init(ROUTE_POST, url, timeout);
    if (client == NULL) {
        response->data(response, http1response_status_string(500));
        return;
    }

    http1request_t* req = client->request;

    jsondoc_t* doc = json_init();
    jsontok_t* object = json_create_object(doc);
    json_object_set(object, "bool", json_create_bool(doc, 1));
    json_object_set(object, "int", json_create_int(doc, 12));
    req->set_payload_json(req, doc);
    json_free(doc);

    http1response_t* res = client->send(client);
    if (!res) {
        response->status_code = 500;
        response->data(response, "error");
        client->free(client);
        return;
    }
    if (res->status_code != 200) {
        response->data(response, http1response_status_string(res->status_code));
        client->free(client);
        return;
    }

    char* document = res->payload(res);
    if (!document) {
        response->status_code = 500;
        response->data(response, "response error");
        client->free(client);
        return;
    }

    response->data(response, document);

    free(document);
    client->free(client);

}
