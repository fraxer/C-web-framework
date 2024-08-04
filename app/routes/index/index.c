#include <linux/limits.h>

#include "http1.h"
#include "websockets.h"
#include "log.h"
#include "json.h"
#include "db.h"
#include "storage.h"
#include "view.h"

void get(httpctx_t* ctx) {
    ctx->response->cookie_add(ctx->response, (cookie_t){
        .name = "mytoken",
        .value = "token_value",
        .minutes = 60,
        .path = "/",
        .secure = 1,
        .http_only = 1,
        .same_site = "Lax"
    });

    ctx->response->data(ctx->response, "done");
}

void post_urlencoded(httpctx_t* ctx) {
    char* data = ctx->request->payloadf(ctx->request, "key1");

    if (data == NULL) {
        ctx->response->data(ctx->response, "payload empty");
        return;
    }

    ctx->response->data(ctx->response, data);
    free(data);
}

void post_formdata(httpctx_t* ctx) {
    char* data = ctx->request->payloadf(ctx->request, "key");

    if (data == NULL) {
        ctx->response->data(ctx->response, "payload empty");
        return;
    }

    ctx->response->data(ctx->response, data);
    free(data);
}

void post(httpctx_t* ctx) {
    jsondoc_t* document = ctx->request->payload_json(ctx->request);
    if (!json_ok(document)) {
        ctx->response->data(ctx->response, json_error(document));
        json_free(document);
        return;
    }

    jsontok_t* object = json_root(document);
    if (!json_is_object(object)) {
        ctx->response->data(ctx->response, "is not object");
        return;
    }

    json_object_set(object, "mykey", json_create_string(document, "Hello"));

    ctx->response->header_add(ctx->response, "Content-Type", "application/json");
    ctx->response->data(ctx->response, json_stringify(document));

    json_free(document);
}

void websocket(httpctx_t* ctx) {
    switch_to_websockets(ctx);
}

void mysql(httpctx_t* ctx) {
    dbinstance_t dbinst = dbinstance("mysql");

    if (!dbinst.ok) {
        ctx->response->data(ctx->response, "db not found");
        return;
    }

    dbresult_t result = dbquery(&dbinst, "select * from check_site ;select * from migration;");

    if (!dbresult_ok(&result)) {
        ctx->response->data(ctx->response, dbresult_error_message(&result));
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

    ctx->response->data(ctx->response, str);

    dbresult_free(&result);
}

void payload(httpctx_t* ctx) {
    char* payload = ctx->request->payload(ctx->request);

    if (payload == NULL) {
        ctx->response->data(ctx->response, "Payload not found");
        return;
    }

    ctx->response->data(ctx->response, payload);

    free(payload);
}

void payloadf(httpctx_t* ctx) {
    char* data = ctx->request->payloadf(ctx->request, "mydata");
    char* text = ctx->request->payloadf(ctx->request, "mytext");

    if (data == NULL) {
        ctx->response->data(ctx->response, "Data not found");
        goto failed;
    }

    if (text == NULL) {
        ctx->response->data(ctx->response, "Text not found");
        goto failed;
    }

    ctx->response->data(ctx->response, text);

    failed:

    if (data) free(data);
    if (text) free(text);
}

void payload_file(httpctx_t* ctx) {
    file_content_t payload_content = ctx->request->payload_file(ctx->request);
    if (!payload_content.ok) {
        ctx->response->data(ctx->response, "file not found");
        return;
    }

    const char* filename = NULL;
    char fullpath[PATH_MAX] = {0};
    snprintf(fullpath, sizeof(fullpath), "%s/%s", ctx->request->connection->server->root, "testdir");
    file_t payload_file = payload_content.make_file(&payload_content, fullpath, filename);
    if (!payload_file.ok) {
        ctx->response->data(ctx->response, "Error save file");
        return;
    }

    char* data = payload_file.content(&payload_file);

    payload_file.close(&payload_file);

    ctx->response->data(ctx->response, data);

    free(data);
}

void payload_filef(httpctx_t* ctx) {
    file_content_t payload_content = ctx->request->payload_filef(ctx->request, "myfile");
    if (!payload_content.ok) {
        ctx->response->data(ctx->response, "file not found");
        return;
    }

    const char* filename = NULL;
    char fullpath[PATH_MAX] = {0};
    snprintf(fullpath, sizeof(fullpath), "%s/%s", ctx->request->connection->server->root, "testdir");
    file_t payload_file = payload_content.make_file(&payload_content, fullpath, filename);
    if (!payload_file.ok) {
        ctx->response->data(ctx->response, "Error save file");
        return;
    }

    char* data = payload_file.content(&payload_file);

    payload_file.close(&payload_file);

    ctx->response->data(ctx->response, data);

    free(data);
}

void payload_jsonf(httpctx_t* ctx) {
    jsondoc_t* document = ctx->request->payload_jsonf(ctx->request, "myjson");

    if (!json_ok(document)) {
        ctx->response->data(ctx->response, json_error(document));
        json_free(document);
        return;
    }

    jsontok_t* object = json_root(document);
    if (!json_is_object(object)) {
        ctx->response->data(ctx->response, "is not object");
        return;
    }

    json_object_set(object, "mykey", json_create_string(document, "Hello"));

    ctx->response->header_add(ctx->response, "Content-Type", "application/json");

    ctx->response->data(ctx->response, json_stringify(document));

    json_free(document);
}

void template_engine(httpctx_t* ctx) {
    file_t file = storage_file_get("views", "/json.txt");
    if (!file.ok) {
        ctx->response->data(ctx->response, "Error read json file");
        return;
    }

    char* data = file.content(&file);
    file.close(&file);

    jsondoc_t* document = json_init();
    if (document == NULL) {
        ctx->response->data(ctx->response, "Json init error");
        free(data);
        return;
    }

    if (!json_parse(document, data)) {
        ctx->response->data(ctx->response, "Json parse error");
        free(data);
        return;
    }

    free(data);

    jsontok_t* object = json_root(document);
    if (!json_is_object(object)) {
        ctx->response->data(ctx->response, "Document is not object");
        return;
    }

    ctx->response->view(ctx->response, document, "views", "/index.tpl");

    json_free(document);
}
