#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "../log/log.h"
#include "../openssl/openssl.h"
#include "../route/route.h"
#include "../request/http1request.h"
#include "../response/http1response.h"
#include "websockets.h"
    #include <stdio.h>

void websockets_parse(connection_t*, unsigned char*, size_t);

void websockets_read(connection_t* connection, char* buffer, size_t buffer_size) {
    while (1) {
        int bytes_readed = websockets_read_internal(connection, buffer, buffer_size);

        printf("%d\n", bytes_readed);

        switch (bytes_readed) {
        case -1:
            // websockets_handle(connection);
            connection->after_read_request(connection);
            return;
        case 0:
            connection->keepalive_enabled = 0;
            connection->after_read_request(connection);
            return;
        default:
            printf("parse websockets\n");

            websockets_parse(connection, (unsigned char*)buffer, buffer_size);

            // if (http1_parser_run(&parser) == -1) {
            //     http1response_default_response((http1response_t*)connection->response, 400);
            //     connection->after_read_request(connection);
            //     return;
            // }
        }
    }
}

void websockets_write(connection_t* connection, char*, size_t) {
    const char* response = "HTTP/1.1 200 OK\r\nServer: TEST\r\nContent-Length: 8\r\nContent-Type: text/html\r\nConnection: Closed\r\n\r\nResponse";

    int size = strlen(response);

    size_t writed = websockets_write_internal(connection, response, size);

    connection->after_write_request(connection);
}

ssize_t websockets_read_internal(connection_t* connection, char* buffer, size_t size) {
    return connection->ssl_enabled ?
        openssl_read(connection->ssl, buffer, size) :
        read(connection->fd, buffer, size);
}

ssize_t websockets_write_internal(connection_t* connection, const char* response, size_t size) {
    if (connection->ssl_enabled) {
        size_t sended = openssl_write(connection->ssl, response, size);

        if (sended == -1) {
            int err = openssl_get_status(connection->ssl, sended);

            // printf("%d\n", err);
        }

        return sended;
    }
        
    return write(connection->fd, response, size);
}

void websockets_parse(connection_t* connection, unsigned char* buffer, size_t buffer_size) {
    unsigned char fin    = (buffer[0] >> 7) & 0x01;
    unsigned char opcode =  buffer[0] & 0x0F;
    unsigned char masked = (buffer[1] >> 7) & 0x01;

    unsigned long long payload_length = 0;
    int pos = 2;
    int length_field = buffer[1] & (~0x80);
    unsigned int mask = 0;

    if (length_field <= 125) {
        payload_length = length_field;
    }
    else if (length_field == 126) {
        payload_length = (
            (buffer[2] << 8) | 
            (buffer[3])
        );
        pos += 2;
    }
    else if (length_field == 127) {
        int byteCount = 8;

        while (--byteCount > 1) {
            payload_length |= (buffer[byteCount + 1] & 0xFF) << (8 * byteCount);
        }

        payload_length |= buffer[9];

        // payload_length = (
        //     (buffer[2] << 56) | 
        //     (buffer[3] << 48) | 
        //     (buffer[4] << 40) | 
        //     (buffer[5] << 32) | 
        //     (buffer[6] << 24) | 
        //     (buffer[7] << 16) | 
        //     (buffer[8] << 8) | 
        //     (buffer[9])
        // );

        pos += 8;
    }

    printf("payload_length: %lld\n", payload_length);

    if (masked) {
        mask = *((unsigned int*)(buffer + pos));
        pos += 4;

        unsigned char* c = buffer + pos;

        for (unsigned long long i = 0; i < payload_length; i++) {
            c[i] = c[i] ^ ((unsigned char*)(&mask))[i % 4];
        }
    }

    char* out_buffer = (char*)malloc(payload_length + 1);

    memcpy(out_buffer, buffer + pos, payload_length);

    out_buffer[payload_length] = 0;

    unsigned short num = ((unsigned char)(out_buffer[0]) | (unsigned char)(out_buffer[1]) << 8);

    // printf("%d\n", num);
    // printf("%s\n", out_buffer);
    // printf("%d, %d\n", (unsigned char)(out_buffer[0]), (unsigned char)(out_buffer[1]));

    printf("num %d\n", num);
    printf("Fin %d\n", fin);
    printf("Opcode %d\n", opcode);
    printf("Masked %d\n", masked);
    printf("%s\n", out_buffer);

    // pong
    if (fin == 1 && opcode == 10) {
        return;
    }

    // continuation frame
    // if (fin == 0 && opcode == 1) {
    //     return;
    // }

    {
        pos = 0;
        unsigned char frame_code = (unsigned char)0x81; // text frame

        char* msg = (char*)malloc(7);

        strcpy(msg, "Hello!");

        int size = strlen(msg);

        {
            char* p = (char*)realloc(msg, payload_length + 1);

            if (!p) {
                if (msg) free(msg);
                return;
            }

            msg = p;

            memcpy(msg, out_buffer + 2, payload_length - 2);

            if (num == 2) {
                frame_code = (unsigned char)0x82;
            }

            size = payload_length - 2;

            // if (rand() % 2 == 0) {
            //     sleep(1);
            // }
        }

        // close connection
        if (fin == 1 && opcode == 8) {
            frame_code = (unsigned char)0x88;

            connection->keepalive_enabled = 0;

            char* p = (char*)realloc(msg, payload_length + 1);

            if (!p) {
                if (msg) free(msg);
                return;
            }

            msg = p;

            memcpy(msg, out_buffer, payload_length);

            size = strlen(msg);
        }

        // ping
        if (fin == 1 && opcode == 9) {
            frame_code = (unsigned char)0x8A;

            char* p = (char*)realloc(msg, payload_length + 1);

            if (!p) {
                if (msg) free(msg);
                return;
            }

            msg = p;

            memcpy(msg, out_buffer, payload_length);

            size = strlen(msg);
        }

        // last frame in queue
        if (fin == 1 && opcode == 0) {}
        
        
        int max_fragment_size = 14;
        unsigned char* buffer = (unsigned char*)malloc(size + max_fragment_size);
        
        buffer[pos++] = frame_code;

        if (size <= 125) {
            buffer[pos++] = size;
        }
        else if (size <= 65535) {
            buffer[pos++] = 126; //16 bit length follows
            
            buffer[pos++] = (size >> 8) & 0xFF; // leftmost first
            buffer[pos++] = size & 0xFF;
        }
        else { // >2^16-1 (65535)
            buffer[pos++] = 127; //64 bit length follows
            
            // write 8 bytes length (significant first)
            
            // since msg_length is int it can be no longer than 4 bytes = 2^32-1
            // padd zeroes for the first 4 bytes
            for(int i=3; i>=0; i--) {
                buffer[pos++] = 0;
            }
            // write the actual 32bit msg_length in the next 4 bytes
            for(int i=3; i>=0; i--) {
                buffer[pos++] = ((size >> 8*i) & 0xFF);
            }
        }

        memcpy(buffer + pos, msg, size);

        printf("%s\n", buffer);
        printf("%d\n", pos);
        printf("%d\n", size);
        printf("%d\n", size + pos);

        // printf("buffer %s\n", buffer);

        // p_obj->response_body = (char*)buffer;
        // p_obj->response_body_length = size + pos;
    }
}

// void wwwrite() {
//     int pos = 0;
//     unsigned char* msg = (unsigned char*)"Hello!";
//     int size = strlen((char*)msg);
//     int maxFragmentSize = 14;
//     unsigned char* buffer = (unsigned char*)malloc(size + maxFragmentSize);

//     buffer[pos++] = (unsigned char)0x81; // text frame

//     if (size <= 125) {
//         buffer[pos++] = size;
//     }
//     else if (size <= 65535) {
//         buffer[pos++] = 126; //16 bit length follows
        
//         buffer[pos++] = (size >> 8) & 0xFF; // leftmost first
//         buffer[pos++] = size & 0xFF;
//     }
//     else { // >2^16-1 (65535)
//         buffer[pos++] = 127; //64 bit length follows
        
//         // write 8 bytes length (significant first)
        
//         // since msg_length is int it can be no longer than 4 bytes = 2^32-1
//         // padd zeroes for the first 4 bytes
//         for(int i=3; i>=0; i--) {
//             buffer[pos++] = 0;
//         }
//         // write the actual 32bit msg_length in the next 4 bytes
//         for(int i=3; i>=0; i--) {
//             buffer[pos++] = ((size >> 8*i) & 0xFF);
//         }
//     }

//     memcpy(buffer + pos, msg, size);

//     printf("%s\n", buffer);
//     printf("%d\n", pos);
//     printf("%d\n", size);
//     printf("%d\n", size + pos);

//     printf("buffer %s\n", buffer);

//     p_obj->response_body = (char*)buffer;
//     p_obj->response_body_length = size + pos;
// }