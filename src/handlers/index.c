#include <openssl/sha.h>
#include <string.h>
#include "../base64/base64.h"
#include "../request/http1request.h"
#include "../response/http1response.h"
#include "../request/websocketsrequest.h"
#include "../response/websocketsresponse.h"
#include "../database/dbquery.h"
#include "../database/dbresult.h"
    #include <stdio.h>

void view(http1request_t* request, http1response_t* response) {
    // printf("run view handler\n");

    const char* data = "Response";

    size_t length = strlen(data);

    // printf("%ld %s\n", length, data);

    // response->header_add(response, "key", "value");
    // response->headern_add(response, "key", 3, "value", 5);

    // response->header_remove(response, "key");
    // response->headern_remove(response, "key", 3);

    response->data(response, data);

    // response->datan(response, data, length);

    // response->file(response, "/darek-zabrocki-mg-tree-town1-003-final-darekzabrocki.jpg"); // path
}

void user(http1request_t* request, http1response_t* response) {
    // printf("run user handler\n");
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
        return response->data(response, "error connect to web socket");
    }

    const char* magic_string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    size_t magic_string_length = strlen(magic_string);

    size_t key_length = ws_key->value_length + magic_string_length;
    size_t pos = 0;
    char key[key_length + 1];

    http1response_data_append(key, &pos, ws_key->value, ws_key->value_length);
    http1response_data_append(key, &pos, magic_string, magic_string_length);

    key[key_length] = 0;

    unsigned char result[40];

    SHA1((const unsigned char*)key, strlen(key), result);

    char pool[41];

    for (int i = 0; i < 20; i++) {
        sprintf(&pool[i*2], "%02x", (unsigned int)result[i]);
    }

    char bs[base64_encode_inline_len(20)];

    int retlen = base64_encode_inline(bs, (const unsigned char*)result, 20);

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

    if (!dbinst.ok) return response->data(response, "db not found");

    dbresult_t result = dbquery(&dbinst, "SET ROLE slave_select; select email from \"user\" limit 3; select id from \"user\" limit 3;");

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

    const db_table_cell_t* field = dbresult_field(&result, "email");


    char* str = (char*)malloc(1024);
    strcpy(str, field->value);

    dbresult_query_next(&result);

    field = dbresult_field(&result, "id");

    strcat(str, " | ");
    strcat(str, field->value);

    dbresult_free(&result);

    response->data(response, str);

    free(str);
}

void db_mysql(http1request_t* request, http1response_t* response) {
    dbinstance_t dbinst = dbinstance(request->database_list(request), "mysql");

    if (!dbinst.ok) return response->data(response, "db not found");

    dbresult_t result = dbquery(&dbinst, "select * from check_site ;select * from migration;");

    if (!dbresult_ok(&result)) {
        response->data(response, dbresult_error_message(&result));
        dbresult_free(&result);
        return;
    }

    while (dbresult_row_next(&result)) {
        const db_table_cell_t* field = dbresult_field(&result, "domain");

        printf("%s\n", field->value);
    }

    dbresult_row_first(&result);
    dbresult_col_first(&result);

    printf("%d\n", dbresult_query_rows(&result));
    printf("%d\n", dbresult_query_cols(&result));
    printf("\n");

    dbresult_query_first(&result);

    do {
        for (int col = 0; col < dbresult_query_cols(&result); col++) {
            printf("%s | ", result.current->fields[col]->value);
        }
        printf("\n");

        for (int row = 0; row < dbresult_query_rows(&result); row++) {
            for (int col = 0; col < dbresult_query_cols(&result); col++) {
                const db_table_cell_t* field = dbresult_cell(&result, row, col);

                printf("%s | ", field->value);
            }
            printf("\n");
        }
        printf("\n");

        dbresult_row_first(&result);
        dbresult_col_first(&result);
    } while (dbresult_query_next(&result));

    dbresult_query_first(&result);


    const db_table_cell_t* field = dbresult_field(&result, "domain");


    char* str = (char*)malloc(1024);
    strcpy(str, field->value);

    dbresult_query_next(&result);

    field = dbresult_cell(&result, 2, 0);

    strcat(str, " | ");
    strcat(str, field->value);

    dbresult_free(&result);

    response->data(response, str);

    free(str);
}
