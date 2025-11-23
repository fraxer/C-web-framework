#include <linux/limits.h>

#include "http.h"
#include "websockets.h"
#include "log.h"
#include "json.h"
#include "db.h"
#include "storage.h"
#include "view.h"
#include "model.h"
#include "auth.h"

void get(httpctx_t* ctx) {
    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "mytoken",
        .value = "token_value",
        .seconds = 3600,
        .path = "/",
        .secure = 1,
        .http_only = 1,
        .same_site = "Lax"
    });

    ctx->response->send_data(ctx->response, "done");
}

void post_urlencoded(httpctx_t* ctx) {
    char* data = ctx->request->get_payloadf(ctx->request, "key1");

    if (data == NULL) {
        ctx->response->send_data(ctx->response, "payload empty");
        return;
    }

    ctx->response->send_data(ctx->response, data);
    free(data);
}

void post_formdata(httpctx_t* ctx) {
    char* data = ctx->request->get_payloadf(ctx->request, "key");

    if (data == NULL) {
        ctx->response->send_data(ctx->response, "payload empty");
        return;
    }

    ctx->response->send_data(ctx->response, data);
    free(data);
}

void post(httpctx_t* ctx) {
    json_doc_t* document = ctx->request->get_payload_json(ctx->request);
    if (document == NULL) {
        ctx->response->send_data(ctx->response, "Payload invalid");
        json_free(document);
        return;
    }

    json_token_t* object = json_root(document);
    if (!json_is_object(object)) {
        ctx->response->send_data(ctx->response, "is not object");
        return;
    }

    json_object_set(object, "mykey", json_create_string("Hello"));

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(document));

    json_free(document);
}

void websocket(httpctx_t* ctx) {
    switch_to_websockets(ctx);
}

void mysql(httpctx_t* ctx) {
    dbresult_t* result = dbqueryf("mysql", "select * from check_site ;select * from migration;", NULL);
    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "query error");
        dbresult_free(result);
        return;
    }

    // do {
    //     const db_table_cell_t* field = dbresult_field(result, "domain");

    //     // printf("%s\n", field->value);
    // }
    // while (dbresult_row_next(result));

    // dbresult_row_first(result);
    // dbresult_col_first(result);

    // // printf("%d\n", dbresult_query_rows(result));
    // // printf("%d\n", dbresult_query_cols(result));
    // // printf("\n");

    // dbresult_query_first(result);

    // do {
    //     for (int col = 0; col < dbresult_query_cols(result); col++) {
    //         // printf("%s | ", result.current->fields[col]->value);
    //     }
    //     // printf("\n");

    //     for (int row = 0; row < dbresult_query_rows(result); row++) {
    //         for (int col = 0; col < dbresult_query_cols(result); col++) {
    //             // printf("%d %d %p\n", row, col, result.current->fields[col]);
    //             const db_table_cell_t* field = dbresult_cell(result, row, col);

    //             // printf("%s (%p) | ", field->value, field);
    //         }
    //         // printf("\n");
    //     }
    //     // printf("\n");

    //     dbresult_row_first(result);
    //     dbresult_col_first(result);
    // } while (dbresult_query_next(result));

    // dbresult_query_first(result);
    // dbresult_row_first(result);
    // dbresult_col_first(result);

    db_table_cell_t* field = dbresult_field(result, "domain");

    char str[1024];
    
    if (field && field->value)
        strcpy(str, field->value);

    dbresult_query_next(result);
    dbresult_row_first(result);
    dbresult_col_first(result);

    field = dbresult_cell(result, 2, 0);

    if (field && field->value) {
        strcat(str, " | ");
        strcat(str, field->value);
    }

    ctx->response->send_data(ctx->response, str);

    dbresult_free(result);
}

void pg(httpctx_t* ctx) {
    // mparams_create_array(arr,
    //     // mparam_int(id, 1235),
    //     mparam_text(name, "John' Doe 2"),
    //     mparam_text(email, "john@example2.com")
    // )
    // dbinsert(dbinst, "public.user", arr);
    // array_free(arr);


    // mparams_create_array(set_arr,
    //     mparam_text(name, "John Doeeee")
    // )
    // mparams_create_array(where_arr,
    //     mparam_int(id, 1234)
    // )
    // dbupdate(dbinst, "public.user", set_arr, where_arr);
    // array_free(set_arr);
    // array_free(where_arr);


    // mparams_create_array(where_arr,
    //     mparam_int(id, 24),
    //     mparam_text(name, "John' Doe 2")
    // )
    // dbdelete(dbinst, "public.user", where_arr);
    // array_free(where_arr);



    // array_t* column_arr = array_create();
    // array_push_back(column_arr, array_create_string("*"));
    // // array_push_back(column_arr, array_create_string("email"));
    // // array_push_back_complex(column_arr, (char*){"name", "email"}, 2);
    // // array_push_back_complex(column_arr, array_batch_strings("name", "email"));

    // mparams_create_array(where_arr,
    //     mparam_int(id, 1234)
    // )
    // dbselect(dbinst, "public.user", column_arr, where_arr);
    // array_free(column_arr);
    // array_free(where_arr);




    array_t* id_arr = array_create_ints(4, 9, 23, 1235, 13377);
    array_t* email_arr = array_create_strings(
        "john@ex\"\'\"ample2.com",
        "john@ex'ample.com",
        "john@ex.com",
        "john@example2.com",
        "pass"
    );

    array_t* fields_arr = array_create_strings("id", "name", "email");

    array_t* arr = array_create();
    if (arr == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        return;
    }
    mparams_fill_array(arr,
        mparam_array(fields, fields_arr),
        mparam_text(scheme, "public"),
        mparam_text(table, "user"),
        mparam_array(id, id_arr),
        mparam_text(name, "John' Doe 2"),
        mparam_array(emails, email_arr)
    )

    dbresult_t* result = dbquery("postgresql", "select @list__fields from @scheme.@table where id in (:list__id) ", arr);
    array_free(arr);
    array_free(email_arr);
    array_free(id_arr);
    array_free(fields_arr);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "query error");
        dbresult_free(result);
        return;
    }

    for (int row = 0; row < dbresult_query_rows(result); row++) {
        for (int col = 0; col < dbresult_query_cols(result); col++) {
            // printf("%d %d %p\n", row, col, result->current->fields[col]);
            const db_table_cell_t* field = dbresult_cell(result, row, col);

            printf("%s | ", field->value);
        }
        printf("\n");
    }

    ctx->response->send_data(ctx->response, "done");

    dbresult_free(result);
}

void redis(httpctx_t* ctx) {
    // dbresult_t* result = dbqueryf("redis.r1", "SET testkey123 123456");
    dbresult_t* result = dbqueryf("redis.r1", "GET testkey123");
    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "error");
        goto failed;
    }

    const db_table_cell_t* field = dbresult_field(result, NULL);

    ctx->response->send_data(ctx->response, field->value);

    failed:

    dbresult_free(result);
}

void payload(httpctx_t* ctx) {
    char* payload = ctx->request->get_payload(ctx->request);

    if (payload == NULL) {
        ctx->response->send_data(ctx->response, "Payload not found");
        return;
    }

    ctx->response->send_data(ctx->response, payload);

    free(payload);
}

void payloadf(httpctx_t* ctx) {
    char* data = ctx->request->get_payloadf(ctx->request, "mydata");
    char* text = ctx->request->get_payloadf(ctx->request, "mytext");

    if (data == NULL) {
        ctx->response->send_data(ctx->response, "Data not found");
        goto failed;
    }

    if (text == NULL) {
        ctx->response->send_data(ctx->response, "Text not found");
        goto failed;
    }

    ctx->response->send_data(ctx->response, text);

    failed:

    if (data) free(data);
    if (text) free(text);
}

void payload_file(httpctx_t* ctx) {
    file_content_t payload_content = ctx->request->get_payload_file(ctx->request);
    if (!payload_content.ok) {
        ctx->response->send_data(ctx->response, "file not found");
        return;
    }

    connection_server_ctx_t* conn_ctx = ctx->request->connection->ctx;

    const char* filename = NULL;
    char fullpath[PATH_MAX] = {0};
    snprintf(fullpath, sizeof(fullpath), "%s/%s", conn_ctx->server->root, "testdir");
    file_t payload_file = payload_content.make_file(&payload_content, fullpath, filename);
    if (!payload_file.ok) {
        ctx->response->send_data(ctx->response, "Error save file");
        return;
    }

    char* data = payload_file.content(&payload_file);

    payload_file.close(&payload_file);

    ctx->response->send_data(ctx->response, data);

    free(data);
}

void payload_filef(httpctx_t* ctx) {
    file_content_t payload_content = ctx->request->get_payload_filef(ctx->request, "myfile");
    if (!payload_content.ok) {
        ctx->response->send_data(ctx->response, "file not found");
        return;
    }

    connection_server_ctx_t* conn_ctx = ctx->request->connection->ctx;

    const char* filename = NULL;
    char fullpath[PATH_MAX] = {0};
    snprintf(fullpath, sizeof(fullpath), "%s/%s", conn_ctx->server->root, "testdir");
    file_t payload_file = payload_content.make_file(&payload_content, fullpath, filename);
    if (!payload_file.ok) {
        ctx->response->send_data(ctx->response, "Error save file");
        return;
    }

    char* data = payload_file.content(&payload_file);

    payload_file.close(&payload_file);

    ctx->response->send_data(ctx->response, data);

    free(data);
}

void payload_jsonf(httpctx_t* ctx) {
    json_doc_t* document = ctx->request->get_payload_jsonf(ctx->request, "myjson");
    if (document == NULL) {
        ctx->response->send_data(ctx->response, "Payload invalid");
        json_free(document);
        return;
    }

    json_token_t* object = json_root(document);
    if (!json_is_object(object)) {
        ctx->response->send_data(ctx->response, "is not object");
        return;
    }

    json_object_set(object, "mykey", json_create_string("Hello"));

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");

    ctx->response->send_data(ctx->response, json_stringify(document));

    json_free(document);
}

void template_engine(httpctx_t* ctx) {
    file_t file = storage_file_get("views", "/json.txt");
    if (!file.ok) {
        ctx->response->send_data(ctx->response, "Error read json file");
        return;
    }

    char* data = file.content(&file);
    file.close(&file);

    json_doc_t* document = json_parse(data);
    if (document == NULL) {
        ctx->response->send_data(ctx->response, "Json parse error");
        free(data);
        return;
    }

    free(data);

    json_token_t* object = json_root(document);
    if (!json_is_object(object)) {
        ctx->response->send_data(ctx->response, "Document is not object");
        return;
    }

    ctx->response->send_view(ctx->response, document, "views", "/index.tpl");

    json_free(document);
}
