#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "../mimetype/mimetype.h"
#include "http1response.h"
    #include <stdio.h>

void http1response_data(http1response_t*, const char*);
void http1response_datan(http1response_t*, const char*, size_t);
int http1response_file(http1response_t*, const char*);
int http1response_filen(http1response_t*, const char*, size_t);
int http1response_header_add(http1response_t*, const char*, const char*);
int http1response_headern_add(http1response_t*, const char*, size_t, const char*, size_t);
const char* http1response_status_string(int);
size_t http1response_status_length(int);
size_t http1response_size(http1response_t*, size_t);
int http1response_data_append(char*, size_t*, const char*, size_t);
int http1response_prepare(http1response_t*, const char*, size_t);
int http1response_header_add_content_length(http1response_t*, size_t);
int http1response_header_add_content_type(http1response_t*, const char*, size_t);
const char* http1response_get_mimetype(const char*);
const char* http1response_get_extention(const char*, size_t);
void http1response_reset(http1response_t*);
int http1response_keepalive_enabled(http1response_t*);


void http1response_header_free(http1_header_t* header) {
    while (header) {
        http1_header_t* next = header->next;

        http1_header_free(header);

        header = next;
    }
}

http1response_t* http1response_alloc() {
    return (http1response_t*)malloc(sizeof(http1response_t));
}

void http1response_free(void* arg) {
    http1response_t* response = (http1response_t*)arg;

    http1response_reset(response);

    free(response);

    response = NULL;
}

http1response_t* http1response_create(connection_t* connection) {
    http1response_t* response = http1response_alloc();

    if (response == NULL) return NULL;

    response->status_code = 200;
    response->body.data = NULL;
    response->body.pos = 0;
    response->body.size = 0;
    response->file_.fd = 0;
    response->file_.pos = 0;
    response->file_.size = 0;
    response->header = NULL;
    response->last_header = NULL;
    response->connection = connection;
    response->data = http1response_data;
    response->datan = http1response_datan;
    response->header_add = http1response_header_add;
    response->headern_add = http1response_headern_add;
    response->header_add_content_length = http1response_header_add_content_length;
    response->header_add_content_type = http1response_header_add_content_type;
    response->header_remove = NULL;
    response->headern_remove = NULL;
    response->file = http1response_file;
    response->filen = http1response_filen;
    response->base.reset = (void(*)(void*))http1response_reset;
    response->base.free = (void(*)(void*))http1response_free;

    return response;
}

void http1response_reset(http1response_t* response) {
    response->status_code = 200;
    response->body.pos = 0;
    response->body.size = 0;

    if (response->file_.fd > 0) {
        lseek(response->file_.fd, 0, SEEK_SET);
        close(response->file_.fd);
    }

    response->file_.fd = 0;
    response->file_.pos = 0;
    response->file_.size = 0;

    if (response->body.data) free(response->body.data);
    response->body.data = NULL;

    http1response_header_free(response->header);
    response->header = NULL;
    response->last_header = NULL;
}

size_t http1response_size(http1response_t* response, size_t body_length) {
    size_t size = 0;

    size += 9; // "HTTP/1.1 "

    size += http1response_status_length(response->status_code);

    http1_header_t* header = response->header;

    while (header) {
        size += header->key_length;
        size += 2; // ": "
        size += header->value_length;
        size += 2; // "\r\n"

        header = header->next;
    }

    size += 2; // "\r\n"

    size += body_length; // body length

    return size;
}

int http1response_data_append(char* data, size_t* pos, const char* string, size_t length) {
    memcpy(&data[*pos], string, length);

    *pos += length;
}

int http1response_prepare(http1response_t* response, const char* body, size_t length) {
    char* data = (char*)malloc(response->body.size);

    if (data == NULL) return -1;

    size_t pos = 0;

    if (http1response_data_append(data, &pos, "HTTP/1.1 ", 9) == -1) return -1;
    if (http1response_data_append(data, &pos, http1response_status_string(response->status_code), http1response_status_length(response->status_code)) == -1) return -1;

    http1_header_t* header = response->header;

    while (header) {
        if (http1response_data_append(data, &pos, header->key, header->key_length) == -1) return -1;
        if (http1response_data_append(data, &pos, ": ", 2) == -1) return -1;
        if (http1response_data_append(data, &pos, header->value, header->value_length) == -1) return -1;
        if (http1response_data_append(data, &pos, "\r\n", 2) == -1) return -1;

        header = header->next;
    }

    if (http1response_data_append(data, &pos, "\r\n", 2) == -1) return -1;

    if (body != NULL && http1response_data_append(data, &pos, body, length) == -1) return -1;

    response->body.data = data;

    return 0;
}

void http1response_data(http1response_t* response, const char* data) {
    http1response_datan(response, data, strlen(data));
}

void http1response_datan(http1response_t* response, const char* data, size_t length) {
    if (http1response_header_add_content_length(response, length) == -1) return;
    if (response->header_add_content_type(response, "text/html; charset=UTF-8", 24) == -1) return;
    if (response->header_add(response, "Connection", http1response_keepalive_enabled(response) ? "Keep-Alive" : "Close") == -1) return;
    if (response->header_add(response, "Cache-Control", "no-store, no-cache") == -1) return;

    response->body.size = http1response_size(response, length);

    if (http1response_prepare(response, data, length) == -1) return;

    // printf("body: %s, %ld\n", response->body.data, response->body.size);
}

int http1response_file(http1response_t* response, const char* path) {
    return http1response_filen(response, path, strlen(path));
}

int http1response_filen(http1response_t* response, const char* path, size_t length) {
    {
        const char* root = response->connection->server->root;
        size_t root_length = response->connection->server->root_length;
        size_t fullpath_length = response->connection->server->root_length + length;
        size_t pos = 0;

        if (path[0] != '/') fullpath_length++;

        char fullpath[fullpath_length + 1];

        http1response_data_append(fullpath, &pos, root, root_length);

        if (path[0] != '/') http1response_data_append(fullpath, &pos, "/", 1);

        http1response_data_append(fullpath, &pos, path, length);

        fullpath[fullpath_length] = 0;

        struct stat stat_obj;

        stat(fullpath, &stat_obj);

        if (S_ISDIR(stat_obj.st_mode)) {
            http1response_default_response(response, 403);
            return -1;
        }

        if (!S_ISREG(stat_obj.st_mode)) {
            http1response_default_response(response, 404);
            return -1;
        }

        response->file_.fd = open(fullpath, O_RDONLY);
    }

    if (response->file_.fd == -1) return -1;

    response->file_.size = (size_t)lseek(response->file_.fd, 0, SEEK_END);

    lseek(response->file_.fd, 0, SEEK_SET);

    const char* ext = http1response_get_extention(path, length);

    const char* mimetype = http1response_get_mimetype(ext);

    if (response->header_add(response, "Connection", http1response_keepalive_enabled(response) ? "Keep-Alive" : "Close") == -1) return -1;
    if (response->header_add_content_type(response, mimetype, strlen(mimetype)) == -1) return -1;
    if (response->header_add_content_length(response, response->file_.size) == -1) return -1;

    response->body.size = http1response_size(response, 0);

    if (http1response_prepare(response, NULL, 0) == -1) return -1;

    // printf("body: %s, %ld\n", response->body.data, response->body.size);

    return 0;
}

int http1response_header_add(http1response_t* response, const char* key, const char* value) {
    return http1response_headern_add(response, key, strlen(key), value, strlen(value));
}

int http1response_headern_add(http1response_t* response, const char* key, size_t key_length, const char* value, size_t value_length) {
    http1_header_t* header = http1_header_create(key, key_length, value, value_length);

    if (header == NULL) return -1;

    if (response->header == NULL) {
        response->header = header;
    }

    if (response->last_header != NULL) {
        response->last_header->next = header;
    }

    response->last_header = header;

    if (header->key == NULL || header->value == NULL) return -1;

    return 0;
}

int http1response_header_add_content_length(http1response_t* response, size_t length) {
    size_t value = length;
    size_t content_length = 0;

    while (value) { content_length++; value /= 10; }

    char content_string[content_length + 1];

    sprintf(content_string, "%ld", length);

    return response->headern_add(response, "Content-Length", 14, content_string, content_length);
}

int http1response_header_add_content_type(http1response_t* response, const char* string, size_t length) {
    return response->headern_add(response, "Content-Type", 12, string, length);
}

const char* http1response_status_string(int status_code) {
    switch (status_code) {
    case 100: return "100 Continue\r\n";
    case 101: return "101 Switching Protocols\r\n";
    case 102: return "102 Processing\r\n";
    case 103: return "103 Early Hints\r\n";
    case 200: return "200 OK\r\n";
    case 201: return "201 Created\r\n";
    case 202: return "202 Accepted\r\n";
    case 203: return "203 Non-Authoritative Information\r\n";
    case 204: return "204 No Content\r\n";
    case 205: return "205 Reset Content\r\n";
    case 206: return "206 Partial Content\r\n";
    case 207: return "207 Multi-Status\r\n";
    case 208: return "208 Already Reported\r\n";
    case 226: return "226 IM Used\r\n";
    case 300: return "300 Multiple Choices\r\n";
    case 301: return "301 Moved Permanently\r\n";
    case 302: return "302 Found\r\n";
    case 303: return "303 See Other\r\n";
    case 304: return "304 Not Modified\r\n";
    case 305: return "305 Use Proxy\r\n";
    case 306: return "306 Switch Proxy\r\n";
    case 307: return "307 Temporary Redirect\r\n";
    case 308: return "308 Permanent Redirect\r\n";
    case 400: return "400 Bad Request\r\n";
    case 401: return "401 Unauthorized\r\n";
    case 402: return "402 Payment Required\r\n";
    case 403: return "403 Forbidden\r\n";
    case 404: return "404 Not Found\r\n";
    case 405: return "405 Method Not Allowed\r\n";
    case 406: return "406 Not Acceptable\r\n";
    case 407: return "407 Proxy Authentication Required\r\n";
    case 408: return "408 Request Timeout\r\n";
    case 409: return "409 Conflict\r\n";
    case 410: return "410 Gone\r\n";
    case 411: return "411 Length Required\r\n";
    case 412: return "412 Precondition Failed\r\n";
    case 413: return "413 Payload Too Large\r\n";
    case 414: return "414 URI Too Long\r\n";
    case 415: return "415 Unsupported Media Type\r\n";
    case 416: return "416 Range Not Satisfiable\r\n";
    case 417: return "417 Expectation Failed\r\n";
    case 418: return "418 I'm a teapot\r\n";
    case 421: return "421 Misdirected Request\r\n";
    case 422: return "422 Unprocessable Entity\r\n";
    case 423: return "423 Locked\r\n";
    case 424: return "424 Failed Dependency\r\n";
    case 426: return "426 Upgrade Required\r\n";
    case 428: return "428 Precondition Required\r\n";
    case 429: return "429 Too Many Requests\r\n";
    case 431: return "431 Request Header Fields Too Large\r\n";
    case 451: return "451 Unavailable For Legal Reasons\r\n";
    case 500: return "500 Internal Server Error\r\n";
    case 501: return "501 Not Implemented\r\n";
    case 502: return "502 Bad Gateway\r\n";
    case 503: return "503 Service Unavailable\r\n";
    case 504: return "504 Gateway Timeout\r\n";
    case 505: return "505 HTTP Version Not Supported\r\n";
    case 506: return "506 Variant Also Negotiates\r\n";
    case 507: return "507 Insufficient Storage\r\n";
    case 508: return "508 Loop Detected\r\n";
    case 510: return "510 Not Extended\r\n";
    case 511: return "511 Network Authentication Required\r\n";
    }

    return NULL;
}

size_t http1response_status_length(int status_code) {
    switch (status_code) {
    case 100: return 14;
    case 101: return 25;
    case 102: return 16;
    case 103: return 17;
    case 200: return 8;
    case 201: return 13;
    case 202: return 14;
    case 203: return 35;
    case 204: return 16;
    case 205: return 19;
    case 206: return 21;
    case 207: return 18;
    case 208: return 22;
    case 226: return 13;
    case 300: return 22;
    case 301: return 23;
    case 302: return 11;
    case 303: return 15;
    case 304: return 18;
    case 305: return 15;
    case 306: return 18;
    case 307: return 24;
    case 308: return 24;
    case 400: return 17;
    case 401: return 18;
    case 402: return 22;
    case 403: return 15;
    case 404: return 15;
    case 405: return 24;
    case 406: return 20;
    case 407: return 35;
    case 408: return 21;
    case 409: return 14;
    case 410: return 10;
    case 411: return 21;
    case 412: return 25;
    case 413: return 23;
    case 414: return 18;
    case 415: return 28;
    case 416: return 27;
    case 417: return 24;
    case 418: return 18;
    case 421: return 25;
    case 422: return 26;
    case 423: return 12;
    case 424: return 23;
    case 426: return 22;
    case 428: return 27;
    case 429: return 23;
    case 431: return 37;
    case 451: return 35;
    case 500: return 27;
    case 501: return 21;
    case 502: return 17;
    case 503: return 25;
    case 504: return 21;
    case 505: return 32;
    case 506: return 29;
    case 507: return 26;
    case 508: return 19;
    case 510: return 18;
    case 511: return 37;
    }

    return 0;
}

const char* http1response_get_mimetype(const char* extension) {
    const char* mimetype = mimetype_find_type(extension);

    if (mimetype == NULL) return "text/plain";

    return mimetype;
}

const char* http1response_get_extention(const char* path, size_t length) {
    for (size_t i = length - 1; i >= 0; i--) {
        switch (path[i]) {
        case '.':
            return &path[i + 1];
        case '/':
            return NULL;
        }
    }

    return NULL;
}

void http1response_default_response(http1response_t* response, int status_code) {
    http1response_reset(response);

    const char* str1 = "<html><head></head><body><h1 style=\"text-align:center;margin:20px\">";
    size_t str1_length = strlen(str1);

    const char* str2 = "</h1></body></html>";
    size_t str2_length = strlen(str2);

    size_t value = status_code;
    size_t content_length = 0;

    while (value) { content_length++; value /= 10; }

    char content_string[content_length + 1];

    sprintf(content_string, "%d", status_code);

    size_t data_length = str1_length + str2_length + content_length;
    size_t pos = 0;

    char data[data_length + 1];

    memcpy(data, str1, str1_length); pos += str1_length;
    memcpy(&data[pos], content_string, content_length); pos += content_length;
    memcpy(&data[pos], str2, str2_length);

    data[data_length] = 0;

    http1response_datan(response, data, data_length);
}

int http1response_keepalive_enabled(http1response_t* response) {
    return response->connection->keepalive_enabled;
}