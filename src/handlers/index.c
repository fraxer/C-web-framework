#include <openssl/sha.h>
#include <string.h>
#include "../base64/base64.h"
#include "../request/http1request.h"
#include "../response/http1response.h"
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
