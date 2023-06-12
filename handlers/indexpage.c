#include <openssl/sha.h>
#include <string.h>
#include "../src/base64/base64.h"
#include "../src/request/http1request.h"
#include "../src/response/http1response.h"
#include "../src/request/websocketsrequest.h"
#include "../src/response/websocketsresponse.h"
#include "../src/database/dbquery.h"
#include "../src/database/dbresult.h"
#include "../src/json/json.h"
    #include <stdio.h>

void payload(http1request_t* request, http1response_t* response) {
    char* payload = request->payload(request);

    if (!payload) {
        response->data(response, "payload not found");
        return;
    }

    response->data(response, payload);
    free(payload);
}

void payloadf(http1request_t* request, http1response_t* response) {
    char* payload = request->payloadf(request, "asd");

    if (!payload) {
        response->data(response, "field not found");
        return;
    }

    response->data(response, payload);
    free(payload);
}

void payload_urlencoded(http1request_t* request, http1response_t* response) {
    char* payload = request->payload_urlencoded(request, "asd");

    if (!payload) {
        response->data(response, "field not found");
        return;
    }

    response->data(response, payload);
    free(payload);
}

void payload_file(http1request_t* request, http1response_t* response) {
    http1_payloadfile_t payloadfile = request->payload_file(request);

    if (!payloadfile.ok) {
        response->data(response, "file not found");
        return;
    }

    char* data = payloadfile.read(&payloadfile);

    response->data(response, data);

    free(data);
}

void payload_filef(http1request_t* request, http1response_t* response) {
    http1_payloadfile_t payloadfile = request->payload_filef(request, "asd");

    if (!payloadfile.ok) {
        response->data(response, "file not found");
        return;
    }

    char* data = payloadfile.read(&payloadfile);

    response->data(response, data);

    free(data);
}

void payload_json(http1request_t* request, http1response_t* response) {
    const char* payload = "{\"c\":4000000123000000,\"name\":\"alex\",\"number\":165.230000,\"bool\":true,\"array\":[\"string\",145,{\"asd\":[]}]}";

    jsondoc_t document;
    if (!json_init(&document)) {
        response->data(response, "json init error");
        return;
    }

    // при повторном парсинге, выдавать ошибку если документ не был освобожден
    if (json_parse(&document, payload) < 0) {
        response->data(response, "json parse error");
        return;
    }

    jsontok_t* object = json_doc_token(&document);
    if (!json_is_object(object)) {
        response->data(response, "is not object");
        return;
    }

    json_object_set(object, "mykey", json_create_null(&document));

    jsontok_t* array = json_create_array(&document);
    json_object_set(object, "prop1", array);

    json_array_append(array, json_create_bool(&document, 1));
    json_array_append(array, json_create_string(&document, "hello kitty"));
    json_array_append_to(array, 0, json_create_number(&document, 123));
    json_array_append_to(array, 2, json_create_number(&document, -4096));
    json_array_append_to(array, 30, json_create_double(&document, -409.6));

    json_array_erase(array, 2, 2);
    json_array_erase(array, 1, 1);

    json_array_prepend(array, json_create_string(&document, "Daddy"));
    jsontok_t* token_double = json_array_get(array, 2);

    json_token_set_double(token_double, 152834534563.132);

    // json_array_clear(array);

    jsontok_t* obj = json_create_object(&document);
    json_array_append(array, obj);

    json_object_set(obj, "key1", json_create_double(&document, 432.9640545));
    json_object_set(obj, "key2", json_create_number(&document, 432765656659640545));
    json_object_set(obj, "key3", json_create_bool(&document, 0));
    json_object_set(obj, "key4", json_create_null(&document));
    json_object_set(obj, "key5", json_create_object(&document));
    json_object_set(obj, "key6", json_create_array(&document));

    jsontok_t* array2 = json_object_get(obj, "key6");
    if (array2)
        json_array_prepend(array2, json_create_string(&document, "Ops"));

    json_object_remove(obj, "key5");
    // json_object_clear(obj);

    for (jsonit_t it = json_init_it(object); !json_end_it(&it); it = json_next_it(&it)) {
        if (strcmp(json_it_key(&it), "number") == 0) {
            const jsontok_t* token = json_it_value(&it);
            if (json_is_double(token)) {
                double bl = json_double(token);
                (void)bl;
            }
        }
    }

    char* string = json_stringify(&document);

    response->data(response, string);
    response->header_add(response, "Content-Type", "application/json");

    free(string);

    json_free(&document);

    // jsmnit_t obit = {
    //     .type = OBJ | ARR,
    //     .end = 0,
    //     .key = token,
    //     .value = NULL
    // };
    // jsmnit_t it = jsmn_init_it(token);

    // do {
        // jsmntok_t* token_value = it.key;
        // jsmntok_t* token_value = it.value;
    // } while (it = jsmn_next_it(it));

    // for (jsmnit_t it = jsmn_init_it(token); !it.end; it = jsmn_next_it(it)) {}


    // jsmntok_t* token_index0_value = jsmn_array_value(token, 0);

    // for (int i = 0; i < jsmn_array_size(token); i++) {
    //     jsmntok_t* token_value = jsmn_array_value(token, i);
    // }
}

void payload_json_post(http1request_t* request, http1response_t* response) {
    jsondoc_t document = request->payload_json(request);
    // jsondoc_t document = request->payload_jsonf(request, "json"); // from multipart

    jsontok_t* object = json_doc_token(&document);
    if (!json_is_object(object)) {
        response->data(response, "is not object");
        return;
    }

    json_object_set(object, "mykey", json_create_null(&document));

    for (jsonit_t it = json_init_it(object); !json_end_it(&it); it = json_next_it(&it)) {
        if (strcmp(json_it_key(&it), "number") == 0) {
            const jsontok_t* token = json_it_value(&it);
            if (json_is_double(token)) {
                double bl = json_double(token);
                (void)bl;
            }
        }
    }

    char* string = json_stringify(&document);

    response->data(response, string);
    response->header_add(response, "Content-Type", "application/json");

    free(string);

    json_free(&document);
}

void cookie(http1request_t* request, http1response_t* response) {
    const char* cookie = request->cookie(request, "test");

    if (!cookie) {
        response->data(response, "cookie not found");
        return;
    }

    response->data(response, cookie);
}

void view(http1request_t* request, http1response_t* response) {
    (void)request;

    const char* data = 
    "Response"
    "123 Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст ОченьОчень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст ОченьОчень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст ОченьОчень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст ОченьОчень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст ОченьОчень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст ОченьОчень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст ОченьОчень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст ОченьОчень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст ОченьОчень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст ОченьОчень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст ОченьОчень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст ОченьОчень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст ОченьОчень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст ОченьОчень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень длинный текст Очень 567"
    ;

    response->header_add(response, "Transfer-Encoding", "chunked");
    response->header_add(response, "Content-Encoding", "gzip");

    size_t length = strlen(data);

    // response->header_add(response, "key", "value");
    // response->headern_add(response, "key", 3, "value", 5);

    // response->header_remove(response, "key");
    // response->headern_remove(response, "key", 3);

    response->datan(response, data, length);

    // response->file(response, "/darek-zabrocki-mg-tree-town1-003-final-darekzabrocki.jpg"); // path
}

void user(http1request_t* request, http1response_t* response) {
    (void)request;
    response->data(response, "Response");
}

void websocket(http1request_t* request, http1response_t* response) {
    /*

    let socket = new WebSocket("wss://dtrack.tech:4443/wss");

    socket.onopen = (event) => {
      socket.send("Here's some text that the server is urgently awaiting!");
    };

    */

    const http1_header_t* connection  = request->headern(request, "Connection", 10);
    const http1_header_t* upgrade     = request->headern(request, "Upgrade", 7);
    const http1_header_t* ws_version  = request->headern(request, "Sec-WebSocket-Version", 21);
    const http1_header_t* ws_key      = request->headern(request, "Sec-WebSocket-Key", 17);
    const http1_header_t* ws_protocol = request->headern(request, "Sec-WebSocket-Protocol", 22);

    if (connection == NULL || upgrade == NULL || ws_version == NULL || ws_key == NULL) {
        response->data(response, "error connect to web socket");
        return;
    }

    const char* magic_string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    size_t magic_string_length = strlen(magic_string);

    size_t key_length = ws_key->value_length + magic_string_length;
    size_t pos = 0;
    char key[key_length + 1];

    memcpy(&key[pos], ws_key->value, ws_key->value_length); pos += ws_key->value_length;
    memcpy(&key[pos], magic_string, magic_string_length); pos += magic_string_length;

    key[key_length] = 0;

    unsigned char result[40];

    SHA1((const unsigned char*)key, strlen(key), result);

    char pool[41];

    for (int i = 0; i < 20; i++) {
        sprintf(&pool[i*2], "%02x", (unsigned int)result[i]);
    }

    char bs[base64_encode_inline_len(20)];

    int retlen = base64_encode_inline(bs, (const char*)result, 20);

    retlen--; // without \0

    response->headern_add(response, "Upgrade", 7, "websocket", 9);
    response->headern_add(response, "Connection", 10, "Upgrade", 7);
    response->headern_add(response, "Sec-WebSocket-Accept", 20, bs, retlen);

    if (ws_protocol != NULL) {
        response->headern_add(response, "Sec-WebSocket-Protocol", 22, ws_protocol->value, ws_protocol->value_length);
    }

    response->status_code = 101;
    response->connection->keepalive_enabled = 1;
    response->switch_to_websockets(response);
}

void ws_index(websocketsrequest_t* request, websocketsresponse_t* response) {
    // printf("run view handler\n");

    char* data = "";

    if (request->payload) {
        data = request->payload;
    }

    size_t length = strlen(data);

    // printf("%ld %s\n", length, data);

    response->textn(response, data, length);

    // response->textn(response, data, length);

    // response->file(response, "/darek-zabrocki-mg-tree-town1-003-final-darekzabrocki.jpg"); // path
}

void db_pg(http1request_t* request, http1response_t* response) {
    dbinstance_t dbinst = dbinstance(request->database_list(request), "postgresql");

    if (!dbinst.ok) {
        response->data(response, "db not found");
        return;
    }

    dbresult_t result = dbquery(&dbinst, "SET ROLE slave_select; select * from \"user\" limit 3; select * from \"room\" limit 4;");

    if (!dbresult_ok(&result)) {
        response->data(response, dbresult_error_message(&result));
        dbresult_free(&result);
        return;
    }

    // do {
    //     while (dbresult_row_next(&result)) {
    //         const db_table_cell_t* field = dbresult_field(&result, "email");

    //         printf("%s\n", field->value);

    //         while (dbresult_col_next(&result)) {
    //             const db_table_cell_t* field = dbresult_field(&result, NULL);
    //         }
    //     }
    // } while (dbresult_query_next(&result));

    // dbresult_query_rows(&result);
    // dbresult_query_cols(&result);

    // for (int row = 0; row < dbresult_query_rows(&result); row++) {
    //     int col = 0;
    //     const db_table_cell_t* field = dbresult_cell(&result, row, col);
    // }

    // dbresult_query_first(&result); // reset data on start position
    // dbresult_row_first(&result);
    // dbresult_col_first(&result);

    char* str = malloc(10240);

    // for (int col = 0; col < dbresult_query_cols(&result); col++) {
    //     strcat(str, result.query->fields[col]->value);
    //     strcat(str, " | ");
    // }

    // dbresult_query_next(&result);

    // for (int col = 0; col < dbresult_query_cols(&result); col++) {
    //     strcat(str, result.query->fields[col]->value);
    //     strcat(str, " | ");
    // }

    // dbresult_query_first(&result);

    // const db_table_cell_t* field = dbresult_field(&result, "email");
    const db_table_cell_t* field = dbresult_cell(&result, 0, 1);

    // for (int row = 0; row < dbresult_query_rows(&result); row++) {
    //     for (int col = 0; col < dbresult_query_cols(&result); col++) {
    //         printf("%d %p\n", row * result.current->cols + col, result.query->table[row * result.current->cols + col]);
    //     }
    // }

    if (field)
        strcpy(str, field->value);

    dbresult_query_next(&result);

    field = dbresult_cell(&result, 2, 6);

    if (field) {
        strcat(str, " | ");
        strcat(str, field->value);
    }

    field = dbresult_cell(&result, 0, 10);

    if (field) {
        strcat(str, " | ");
        strcat(str, field->value);
    }

    field = dbresult_cell(&result, 2, 11);

    if (field) {
        strcat(str, " | ");
        strcat(str, field->value);
    }

    field = dbresult_cell(&result, 1, 12);

    if (field) {
        strcat(str, " | ");
        strcat(str, field->value);
    }

    dbresult_free(&result);

    response->data(response, str);

    free(str);

}

void db_mysql(http1request_t* request, http1response_t* response) {
    dbinstance_t dbinst = dbinstance(request->database_list(request), "mysql");

    if (!dbinst.ok) {
        response->data(response, "db not found");
        return;
    }

    dbresult_t result = dbquery(&dbinst, "select * from check_site ;select * from migration;");

    if (!dbresult_ok(&result)) {
        response->data(response, dbresult_error_message(&result));
        dbresult_free(&result);
        return;
    }

    // do {
    //     const db_table_cell_t* field = dbresult_field(&result, "domain");

    //     // printf("%s\n", field->value);
    // }
    // while (dbresult_row_next(&result));

    // dbresult_row_first(&result);
    // dbresult_col_first(&result);

    // // printf("%d\n", dbresult_query_rows(&result));
    // // printf("%d\n", dbresult_query_cols(&result));
    // // printf("\n");

    // dbresult_query_first(&result);

    // do {
    //     for (int col = 0; col < dbresult_query_cols(&result); col++) {
    //         // printf("%s | ", result.current->fields[col]->value);
    //     }
    //     // printf("\n");

    //     for (int row = 0; row < dbresult_query_rows(&result); row++) {
    //         for (int col = 0; col < dbresult_query_cols(&result); col++) {
    //             // printf("%d %d %p\n", row, col, result.current->fields[col]);
    //             const db_table_cell_t* field = dbresult_cell(&result, row, col);

    //             // printf("%s (%p) | ", field->value, field);
    //         }
    //         // printf("\n");
    //     }
    //     // printf("\n");

    //     dbresult_row_first(&result);
    //     dbresult_col_first(&result);
    // } while (dbresult_query_next(&result));

    // dbresult_query_first(&result);
    // dbresult_row_first(&result);
    // dbresult_col_first(&result);


    db_table_cell_t* field = dbresult_field(&result, "domain");


    char* str = (char*)malloc(1024);
    // strcpy(str, "test");
    
    if (field && field->value) {
        strcpy(str, field->value);
    }

    dbresult_query_next(&result);
    dbresult_row_first(&result);
    dbresult_col_first(&result);

    field = dbresult_cell(&result, 2, 0);

    if (field && field->value) {
        strcat(str, " | ");
        strcat(str, field->value);
    }

    response->data(response, str);

    dbresult_free(&result);

    free(str);
}

void db_redis(http1request_t* request, http1response_t* response) {
    dbinstance_t dbinst = dbinstance(request->database_list(request), "redis");

    if (!dbinst.ok) {
        response->data(response, "db not found");
        return;
    }

    // dbresult_t result = dbquery(&dbinst, "SET testkey 7978979");
    dbresult_t result = dbquery(&dbinst, "GET testkey");
    // dbresult_t result = dbquery(&dbinst, "GET key_bool");
    // dbresult_t result = dbquery(&dbinst, "JSON.GET json");
    // dbresult_t result = dbquery(&dbinst, "LRANGE list 0 -1");

    if (!dbresult_ok(&result)) {
        response->data(response, dbresult_error_message(&result));
        dbresult_free(&result);
        return;
    }

    const db_table_cell_t* field = dbresult_field(&result, NULL);

    response->datan(response, field->value, field->length);

    dbresult_free(&result);
}
