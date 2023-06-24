#ifndef __WEBSOCKETSSWITCH__
#define __WEBSOCKETSSWITCH__

#include <string.h>

#include "http1response.h"
#include "http1request.h"
#include "sha1.h"
#include "base64.h"

void switch_to_websockets(http1request_t* request, http1response_t* response) {
    const http1_header_t* connection  = request->headern(request, "Connection", 10);
    const http1_header_t* upgrade     = request->headern(request, "Upgrade", 7);
    const http1_header_t* ws_version  = request->headern(request, "Sec-WebSocket-Version", 21);
    const http1_header_t* ws_key      = request->headern(request, "Sec-WebSocket-Key", 17);
    const http1_header_t* ws_protocol = request->headern(request, "Sec-WebSocket-Protocol", 22);

    if (connection == NULL || upgrade == NULL || ws_version == NULL || ws_key == NULL) {
        response->data(response, "error connect to web socket");
        return;
    }

    char key[128] = {0};
    const char* magic_string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    strcpy(key, ws_key->value);
    strcat(key, magic_string);

    unsigned char result[40];
    sha1((const unsigned char*)key, strlen(key), result);

    char base64_string[base64_encode_inline_len(20)];
    int retlen = base64_encode_inline(base64_string, (const char*)result, 20);
    retlen--; // without \0

    response->headern_add(response, "Upgrade", 7, "websocket", 9);
    response->headern_add(response, "Connection", 10, "Upgrade", 7);
    response->headern_add(response, "Sec-WebSocket-Accept", 20, base64_string, retlen);

    if (ws_protocol != NULL) {
        response->headern_add(response, "Sec-WebSocket-Protocol", 22, ws_protocol->value, ws_protocol->value_length);
    }

    response->status_code = 101;
    response->connection->keepalive_enabled = 1;
    response->switch_to_websockets(response);
}

#endif