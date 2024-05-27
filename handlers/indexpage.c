#include <linux/limits.h>

#include "http1.h"
#include "websockets.h"
#include "log.h"
#include "json.h"
#include "db.h"
#include "storage.h"
#include "view.h"

void get(http1request_t* request, http1response_t* response) {
    (void)request;

    response->cookie_add(response, (cookie_t){
        .name = "mytoken",
        .value = "token_value",
        .minutes = 60,
        .path = "/",
        .secure = 1,
        .http_only = 1,
        .same_site = "Lax"
    });

    response->data(response, "done");
}

void post_urlencoded(http1request_t* request, http1response_t* response) {
    char* data = request->payloadf(request, "key1");

    if (data == NULL) {
        response->data(response, "payload empty");
        return;
    }

    response->data(response, data);
    free(data);
}

void post_formdata(http1request_t* request, http1response_t* response) {
    char* data = request->payloadf(request, "key");

    if (data == NULL) {
        response->data(response, "payload empty");
        return;
    }

    response->data(response, data);
    free(data);
}

void post(http1request_t* request, http1response_t* response) {
    jsondoc_t* document = request->payload_json(request);
    if (!json_ok(document)) {
        response->data(response, json_error(document));
        json_free(document);
        return;
    }

    jsontok_t* object = json_root(document);
    if (!json_is_object(object)) {
        response->data(response, "is not object");
        return;
    }

    json_object_set(object, "mykey", json_create_string(document, "Hello"));

    response->header_add(response, "Content-Type", "application/json");
    response->data(response, json_stringify(document));

    json_free(document);
}

void websocket(http1request_t* request, http1response_t* response) {
    switch_to_websockets(request, response);
}

void mysql(http1request_t* request, http1response_t* response) {
    (void)request;
    dbinstance_t dbinst = dbinstance("mysql");

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

    char str[1024];
    
    if (field && field->value)
        strcpy(str, field->value);

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
}

void payload(http1request_t* request, http1response_t* response) {
    char* payload = request->payload(request);

    if (payload == NULL) {
        response->data(response, "Payload not found");
        return;
    }

    response->data(response, payload);

    free(payload);
}

void payloadf(http1request_t* request, http1response_t* response) {
    char* data = request->payloadf(request, "mydata");
    char* text = request->payloadf(request, "mytext");

    if (data == NULL) {
        response->data(response, "Data not found");
        goto failed;
    }

    if (text == NULL) {
        response->data(response, "Text not found");
        goto failed;
    }

    response->data(response, text);

    failed:

    if (data) free(data);
    if (text) free(text);
}

void payload_file(http1request_t* request, http1response_t* response) {
    file_content_t payload_content = request->payload_file(request);
    if (!payload_content.ok) {
        response->data(response, "file not found");
        return;
    }

    const char* filename = NULL;
    char fullpath[PATH_MAX] = {0};
    snprintf(fullpath, sizeof(fullpath), "%s/%s", request->connection->server->root, "testdir");
    file_t payload_file = payload_content.make_file(&payload_content, fullpath, filename);
    if (!payload_file.ok) {
        response->data(response, "Error save file");
        return;
    }

    char* data = payload_file.content(&payload_file);

    payload_file.close(&payload_file);

    response->data(response, data);

    free(data);
}

void payload_filef(http1request_t* request, http1response_t* response) {
    file_content_t payload_content = request->payload_filef(request, "myfile");
    if (!payload_content.ok) {
        response->data(response, "file not found");
        return;
    }

    const char* filename = NULL;
    char fullpath[PATH_MAX] = {0};
    snprintf(fullpath, sizeof(fullpath), "%s/%s", request->connection->server->root, "testdir");
    file_t payload_file = payload_content.make_file(&payload_content, fullpath, filename);
    if (!payload_file.ok) {
        response->data(response, "Error save file");
        return;
    }

    char* data = payload_file.content(&payload_file);

    payload_file.close(&payload_file);

    response->data(response, data);

    free(data);
}

void payload_jsonf(http1request_t* request, http1response_t* response) {
    jsondoc_t* document = request->payload_jsonf(request, "myjson");

    if (!json_ok(document)) {
        response->data(response, json_error(document));
        json_free(document);
        return;
    }

    jsontok_t* object = json_root(document);
    if (!json_is_object(object)) {
        response->data(response, "is not object");
        return;
    }

    json_object_set(object, "mykey", json_create_string(document, "Hello"));

    response->header_add(response, "Content-Type", "application/json");

    response->data(response, json_stringify(document));

    json_free(document);
}

void template_engine(__attribute__((unused))http1request_t* request, http1response_t* response) {
    file_t file = storage_file_get("views", "/json.txt");
    if (!file.ok) {
        response->data(response, "error");
        return;
    }

    char* data = file.content(&file);
    file.close(&file);

    jsondoc_t* document = json_init();
    if (!document) {
        response->data(response, "json init error");
        return;
    }

    if (json_parse(document, data) < 0) {
        response->data(response, "json parse error");
        return;
    }

    free(data);

    jsontok_t* object = json_root(document);
    if (!json_is_object(object)) {
        response->data(response, "is not object");
        return;
    }

    // response->view(response, document, "views", "/index.tpl");

    // for (int i = 0 ; i < 100; i++) {
    //     char* content = render(document, "views", "/index.tpl");
    //     free(content);
    // }

    char* content = render(document, "views", "/index.tpl");
    response->data(response, content);

    free(content);
    json_free(document);
}
