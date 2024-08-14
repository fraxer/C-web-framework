#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>

#include "log.h"
#include "helpers.h"
#include "json.h"
#include "appconfig.h"
#include "mimetype.h"
#include "http1response.h"
#include "http1responseparser.h"
#include "storage.h"
#include "view.h"

static void __http1response_data(http1response_t* response, const char* data);
static void __http1response_datan(http1response_t* response, const char* data, size_t length);
static void __http1response_file(http1response_t* response, const char* path);
static void __http1response_filen(http1response_t* response, const char* path, size_t length);
static void __http1response_filef(http1response_t*, const char*, const char*, ...);
static void __http1response_default(http1response_t* response, int status_code);
static http1_header_t* __http1response_header_get(http1response_t* response, const char* key);
static int __http1response_header_add(http1response_t* response, const char* key, const char* value);
static int __http1response_headern_add(http1response_t* response, const char* key, size_t key_length, const char* value, size_t value_length);
static int __http1response_headeru_add(http1response_t* response, const char* key, size_t key_length, const char* value, size_t value_length);
static int __http1response_header_exist(http1response_t* response, const char* key);
static size_t __http1response_status_length(int status_code);
static size_t __http1response_head_size(http1response_t* response);
static int __http1response_alloc_body(http1response_t* response, const char* data, size_t length);
static int __http1response_header_add_content_length(http1response_t* response, size_t length);
static const char* __http1response_get_mimetype(const char* extension);

static int __http1response_keepalive_enabled(http1response_t* response);
static void __http1response_try_enable_gzip(http1response_t* response, const char* directive);
static void __http1response_try_enable_te(http1response_t* response, const char* directive);
static int __http1response_prepare_body(http1response_t* response, size_t length);
static void __http1response_cookie_add(http1response_t* response, cookie_t cookie);
static void __http1response_payload_free(http1_payload_t* payload);
static void __http1response_init_payload(http1response_t* response);
static void __http1response_payload_parse_plain(http1response_t* response);
static char* __http1response_payload(http1response_t* response);
static file_content_t __http1response_payload_file(http1response_t* response);
static jsondoc_t* __http1response_payload_json(http1response_t* response);

static void __http1response_free(void* arg);
static int __http1response_init_parser(http1response_t* response);
static void __http1response_reset(http1response_t* response);
static int __http1response_data_append(char* data, size_t* pos, const char* string, size_t length);

void __http1response_view(http1response_t* response, jsondoc_t* document, const char* storage_name, const char* path_format, ...);

void __http1response_free(void* arg) {
    http1response_t* response = arg;

    __http1response_reset(response);
    http1responseparser_free(response->parser);

    free(response);
}

int __http1response_init_parser(http1response_t* response) {
    response->parser = malloc(sizeof(http1responseparser_t));
    if (response->parser == NULL) return 0;

    http1responseparser_init(response->parser);

    return 1;
}

void __http1response_view(http1response_t* response, jsondoc_t* document, const char* storage_name, const char* path_format, ...)
{
    char path[PATH_MAX];

    va_list args;
    va_start(args, path_format);
    vsnprintf(path, sizeof(path), path_format, args);
    va_end(args);

    char* data = render(document, storage_name, path);
    if (data == NULL) {
        response->data(response, "Render view error");
        return;
    }

    response->data(response, data);

    free(data);
}

http1response_t* http1response_create(connection_t* connection) {
    http1response_t* response = malloc(sizeof * response);
    if (response == NULL) return NULL;

    response->status_code = 200;
    response->version = HTTP1_VER_NONE;
    response->head_writed = 0;
    response->transfer_encoding = TE_NONE;
    response->content_encoding = CE_NONE;
    response->content_length = 0;
    response->body.data = NULL;
    response->body.pos = 0;
    response->body.size = 0;
    response->file_ = file_alloc();
    response->file_pos = 0;
    response->header_ = NULL;
    response->last_header = NULL;
    response->ranges = NULL;
    response->connection = connection;
    response->data = __http1response_data;
    response->datan = __http1response_datan;
    response->view = __http1response_view;
    response->def = __http1response_default;
    response->redirect = http1response_redirect;
    response->header = __http1response_header_get;
    response->header_add = __http1response_header_add;
    response->headern_add = __http1response_headern_add;
    response->headeru_add = __http1response_headeru_add;
    response->header_add_content_length = __http1response_header_add_content_length;
    response->header_remove = NULL;
    response->headern_remove = NULL;
    response->file = __http1response_file;
    response->filen = __http1response_filen;
    response->filef = __http1response_filef;
    response->cookie_add = __http1response_cookie_add;
    response->base.reset = (void(*)(void*))__http1response_reset;
    response->base.free = (void(*)(void*))__http1response_free;

    __http1response_init_payload(response);

    if (!__http1response_init_parser(response)) {
        free(response);
        return NULL;
    }

    return response;
}

void __http1response_reset(http1response_t* response) {
    response->status_code = 200;
    response->head_writed = 0;
    response->transfer_encoding = TE_NONE;
    response->content_encoding = CE_NONE;
    response->body.pos = 0;
    response->body.size = 0;

    __http1response_payload_free(&response->payload_);

    response->file_.close(&response->file_);
    response->file_pos = 0;

    if (response->body.data) free(response->body.data);
    response->body.data = NULL;

    http1_headers_free(response->header_);
    response->header_ = NULL;
    response->last_header = NULL;

    http1response_free_ranges(response->ranges);
    response->ranges = NULL;

    http1responseparser_reset(response->parser);
}

size_t __http1response_head_size(http1response_t* response) {
    size_t size = 0;

    size += 9; // "HTTP/X.X "

    size += __http1response_status_length(response->status_code);

    http1_header_t* header = response->header_;

    while (header) {
        size += header->key_length;
        size += 2; // ": "
        size += header->value_length;
        size += 2; // "\r\n"

        header = header->next;
    }

    size += 2; // "\r\n"

    return size;
}

int __http1response_data_append(char* data, size_t* pos, const char* string, size_t length) {
    if (data == NULL) return 0;

    memcpy(&data[*pos], string, length);
    *pos += length;

    return 1;
}

void __http1response_data(http1response_t* response, const char* data) {
    __http1response_datan(response, data, strlen(data));
}

void __http1response_datan(http1response_t* response, const char* data, size_t length) {
    const char* connection = __http1response_keepalive_enabled(response) ? "keep-alive" : "close";
    response->headeru_add(response, "Content-Type", 12, "text/html; charset=utf-8", 24);
    response->headeru_add(response, "Connection", 10, connection, strlen(connection));
    response->headeru_add(response, "Cache-Control", 13, "no-store, no-cache", 18);

    if (!__http1response_prepare_body(response, length)) {
        response->def(response, 500);
        return;
    }

    if (!__http1response_alloc_body(response, data, length))
        response->def(response, 500);
}

void __http1response_file(http1response_t* response, const char* path) {
    __http1response_filen(response, path, strlen(path));
}

void __http1response_filen(http1response_t* response, const char* path, size_t length) {
    const char* root = response->connection->server->root;
    size_t root_length = response->connection->server->root_length;
    size_t fullpath_length = response->connection->server->root_length + length;

    if (path[0] != '/') fullpath_length++;

    char fullpath[fullpath_length + 1];

    index_t* index = response->connection->server->index;

    size_t indexpath_length = fullpath_length;

    if (index != NULL) indexpath_length += index->length;

    if (path[length - 1] != '/') indexpath_length++;

    char indexpath[indexpath_length + 1];
    char* resultpath = fullpath;

    {
        size_t pos = 0;

        __http1response_data_append(fullpath, &pos, root, root_length);

        if (path[0] != '/') __http1response_data_append(fullpath, &pos, "/", 1);

        __http1response_data_append(fullpath, &pos, path, length);

        fullpath[fullpath_length] = 0;

        struct stat stat_obj;
        if (stat(fullpath, &stat_obj) == -1 && errno == ENOENT) {
            response->def(response, 404);
            return;
        }

        if (S_ISDIR(stat_obj.st_mode)) {
            if (index == NULL) {
                response->def(response, 403);
                return;
            }

            size_t index_pos = 0;

            __http1response_data_append(indexpath, &index_pos, fullpath, fullpath_length);

            if (fullpath[fullpath_length - 1] != '/') {
                __http1response_data_append(indexpath, &index_pos, "/", 1);
            }

            __http1response_data_append(indexpath, &index_pos, index->value, index->length);

            indexpath[indexpath_length] = 0;

            if (stat(indexpath, &stat_obj) == -1 && errno == ENOENT) {
                response->def(response, 404);
                return;
            }
            if (!S_ISREG(stat_obj.st_mode)) {
                response->def(response, 403);
                return;
            }

            resultpath = indexpath;
        }
        else if (!S_ISREG(stat_obj.st_mode)) {
            response->def(response, 404);
            return;
        }
    }

    response->file_ = file_open(resultpath, O_RDONLY);
    if (!response->file_.ok) {
        response->def(response, 404);
        return;
    }

    const char* ext = file_extention(resultpath);
    const char* mimetype = __http1response_get_mimetype(ext);
    const char* connection = __http1response_keepalive_enabled(response) ? "keep-alive" : "close";
    response->headeru_add(response, "Connection", 10, connection, strlen(connection));
    response->headeru_add(response, "Content-Type", 12, mimetype, strlen(mimetype));

    if (!__http1response_prepare_body(response, response->file_.size))
        response->def(response, 500);
}

void __http1response_filef(http1response_t* response, const char* storage_name, const char* path_format, ...) {
    char path[PATH_MAX];
    va_list args;
    va_start(args, path_format);
    vsnprintf(path, sizeof(path), path_format, args);
    va_end(args);

    response->file_ = storage_file_get(storage_name, path);
    if (!response->file_.ok) {
        response->def(response, 404);
        return;
    }

    const char* ext = file_extention(response->file_.name);
    const char* mimetype = __http1response_get_mimetype(ext);
    const char* connection = __http1response_keepalive_enabled(response) ? "keep-alive" : "close";
    response->headeru_add(response, "Connection", 10, connection, strlen(connection));
    response->headeru_add(response, "Content-Type", 12, mimetype, strlen(mimetype));

    if (!__http1response_prepare_body(response, response->file_.size))
        response->def(response, 500);
}

http1_header_t* __http1response_header_get(http1response_t* response, const char* key) {
    http1_header_t* header = response->header_;
    while (header != NULL) {
        if (cmpstr_lower(header->key, key))
            return header;

        header = header->next;
    }

    return NULL;
}

int __http1response_header_add(http1response_t* response, const char* key, const char* value) {
    return __http1response_headern_add(response, key, strlen(key), value, strlen(value));
}

int __http1response_headern_add(http1response_t* response, const char* key, size_t key_length, const char* value, size_t value_length) {
    if (response->ranges &&
        (cmpstr_lower(key, "Transfer-Encoding") ||
         cmpstr_lower(key, "Content-Encoding")
    )) {
        return 1;
    }
    
    http1_header_t* header = http1_header_create(key, key_length, value, value_length);
    if (header == NULL) return 0;
    if (header->key == NULL || header->value == NULL) {
        http1_header_free(header);
        return 0;
    }

    if (response->header_ == NULL)
        response->header_ = header;

    if (response->last_header != NULL)
        response->last_header->next = header;

    response->last_header = header;

    if (!response->ranges) {
        if (cmpstr_lower(header->key, "Content-Type")) {
            __http1response_try_enable_gzip(response, header->value);
        }
        else if (cmpstr_lower(header->key, "Transfer-Encoding")) {
            __http1response_try_enable_te(response, header->value);
        }
        else if (cmpstr_lower(header->key, "Content-Encoding") &&
            cmpstr_lower(header->value, "gzip")) {
            response->content_encoding = CE_GZIP;
        }
    }

    return 1;
}

int __http1response_headeru_add(http1response_t* response, const char* key, size_t key_length, const char* value, size_t value_length) {
    if (!__http1response_header_exist(response, key))
        return __http1response_headern_add(response, key, key_length, value, value_length);

    return 0;
}

int __http1response_header_exist(http1response_t* response, const char* key) {
    http1_header_t* header = response->header_;
    while (header) {
        if (cmpstr_lower(header->key, key))
            return 1;

        header = header->next;
    }

    return 0;
}

int __http1response_header_add_content_length(http1response_t* response, size_t length) {
    if (length == 0)
        return response->headern_add(response, "Content-Length", 14, "0", 1);

    size_t value = length;
    size_t content_length = 0;
    while (value) { content_length++; value /= 10; }

    char content_string[content_length + 1];
    sprintf(content_string, "%ld", length);

    return response->headern_add(response, "Content-Length", 14, content_string, content_length);
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

size_t __http1response_status_length(int status_code) {
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

const char* __http1response_get_mimetype(const char* extension) {
    const char* mimetype = mimetype_find_type(appconfig()->mimetype, extension);

    return mimetype == NULL ? "text/plain" : mimetype;
}

void __http1response_default(http1response_t* response, int status_code) {
    response->status_code = status_code;

    const char* str1 = "<html><head></head><body style=\"text-align:center;margin:20px\"><h1>";
    size_t str1_length = strlen(str1);

    const char* str2 = "</h1></body></html>";
    size_t str2_length = strlen(str2);

    size_t status_code_length = __http1response_status_length(status_code) - 2;
    size_t data_length = str1_length + status_code_length + str2_length;
    size_t pos = 0;

    char data[data_length + 1];
    const char* status_code_string = http1response_status_string(status_code);

    memcpy(data, str1, str1_length); pos += str1_length;
    memcpy(&data[pos], status_code_string, status_code_length); pos += status_code_length;
    memcpy(&data[pos], str2, str2_length);

    data[data_length] = 0;

    response->datan(response, data, data_length);
}

int __http1response_keepalive_enabled(http1response_t* response) {
    return response->connection->keepalive_enabled;
}

void http1response_redirect(http1response_t* response, const char* path, int status_code) {
    response->status_code = status_code;

    if (!response->header_add(response, "Location", path))
        return;

    if (http1response_redirect_is_external(path))
        response->header_add(response, "Connection", "Close");
}

http1response_head_t http1response_create_head(http1response_t* response) {
    http1response_head_t head = {
        .size = __http1response_head_size(response),
        .data = malloc(head.size)
    };

    size_t pos = 0;
    if (!__http1response_data_append(head.data, &pos, "HTTP/1.1 ", 9)) return head;
    if (!__http1response_data_append(head.data, &pos, http1response_status_string(response->status_code), __http1response_status_length(response->status_code))) return head;

    http1_header_t* header = response->header_;
    while (header) {
        if (!__http1response_data_append(head.data, &pos, header->key, header->key_length)) return head;
        if (!__http1response_data_append(head.data, &pos, ": ", 2)) return head;
        if (!__http1response_data_append(head.data, &pos, header->value, header->value_length)) return head;
        if (!__http1response_data_append(head.data, &pos, "\r\n", 2)) return head;

        header = header->next;
    }

    if (!__http1response_data_append(head.data, &pos, "\r\n", 2)) return head;

    return head;
}

int __http1response_alloc_body(http1response_t* response, const char* data, size_t length) {
    response->body.data = malloc(length);
    if (response->body.data == NULL) return 0;

    memcpy(response->body.data, data, length);
    response->body.size = length;

    return 1;
}

void __http1response_try_enable_gzip(http1response_t* response, const char* directive) {
    env_gzip_str_t* item = env()->main.gzip;
    while (item != NULL) {
        if (cmpstr_lower(item->mimetype, directive)) {
            response->content_encoding = CE_GZIP;
            break;
        }
        item = item->next;
    }
}

void __http1response_try_enable_te(http1response_t* response, const char* directive) {
    if (cmpstr_lower(directive, "chunked"))
        response->transfer_encoding = TE_CHUNKED;
}

http1_ranges_t* http1response_init_ranges(void) {
    http1_ranges_t* range = malloc(sizeof * range);
    if (range == NULL) return NULL;

    range->start = -1;
    range->end = -1;
    range->pos = 0;
    range->next = NULL;

    return range;
}

void http1response_free_ranges(http1_ranges_t* ranges) {
    while (ranges) {
        http1_ranges_t* next = ranges->next;

        free(ranges);

        ranges = next;
    }
}

int __http1response_prepare_body(http1response_t* response, size_t length) {
    response->headeru_add(response, "Accept-Ranges", 13, "bytes", 5);

    if (!response->ranges) {
        if (response->content_encoding == CE_GZIP) {
            if (!response->headeru_add(response, "Transfer-Encoding", 17, "chunked", 7)) return 0;
            if (!response->headeru_add(response, "Content-Encoding", 16, "gzip", 4)) return 0;
        }

        if (response->transfer_encoding != TE_CHUNKED) {
            if (!__http1response_header_exist(response, "Content-Length"))
                if (!__http1response_header_add_content_length(response, length)) return 0;
        }
    }
    // one range
    else if (!response->ranges->next) {
        if (response->ranges->start > (ssize_t)length) {
            http1response_free_ranges(response->ranges);
            response->ranges = NULL;
            response->def(response, 416);
            return 0;
        }

        response->status_code = 206;

        ssize_t end = response->ranges->end > (ssize_t)length ? (ssize_t)length : response->ranges->end + 1;
        if (response->ranges->end == -1) end = length;

        ssize_t start = response->ranges->start;
        if (response->ranges->start == -1) {
            end = length;
            start = (ssize_t)length - response->ranges->end;
        }

        char bytes[70] = {0};
        int size = snprintf(bytes, sizeof(bytes), "bytes %ld-%ld/%ld", start, end - 1, length);

        if (!response->headeru_add(response, "Content-Range", 13, bytes, size)) return 0;
        if (!__http1response_header_add_content_length(response, end - start)) return 0;
    }
    // many ranges
    else if (response->ranges->next) {
        // TODO
    }

    return 1;
}

void __http1response_cookie_add(http1response_t* response, cookie_t cookie) {
    if (cookie.name == NULL || cookie.name[0] == 0)
        return;
    if (cookie.value == NULL || cookie.value[0] == 0)
        return;

    char template[128];
    char date[32];
    strcpy(template, "%s=%s");
    int count = 2;
    const char* vars[4];

    if (cookie.minutes > 0) {
        vars[count - 2] = date;
        count++;
        time_t t = time(NULL);
        t += cookie.minutes * 60;
        struct tm* timeptr = localtime(&t);

        if (strftime(date, sizeof(date), "%a, %d %b %Y %T GMT", timeptr) == 0) return;

        strcat(template, "; Expires=%s");
    }
    if (cookie.path != NULL && cookie.path[0] != 0) {
        vars[count - 2] = cookie.path;
        count++;
        strcat(template, "; Path=%s");
    }
    if (cookie.domain != NULL && cookie.domain[0] != 0) {
        vars[count - 2] = cookie.domain;
        count++;
        strcat(template, "; Domain=%s");
    }
    if (cookie.secure) strcat(template, "; Secure");
    if (cookie.http_only) strcat(template, "; HttpOnly");
    if (cookie.same_site != NULL && cookie.same_site[0] != 0) {
        vars[count - 2] = cookie.same_site;
        count++;
        strcat(template, "; SameSite=%s");
    }

    char* string = NULL;

    switch (count) {
    case 2:
    {
        size_t string_length = snprintf(NULL, 0, template, cookie.name, cookie.value) + 1;
        string = malloc(string_length);
        if (string == NULL) return;
        snprintf(string, string_length, template, cookie.name, cookie.value);
        break;
    }
    case 3:
    {
        size_t string_length = snprintf(NULL, 0, template, cookie.name, cookie.value, vars[0]) + 1;
        string = malloc(string_length);
        if (string == NULL) return;
        snprintf(string, string_length, template, cookie.name, cookie.value, vars[0]);
        break;
    }
    case 4:
    {
        size_t string_length = snprintf(NULL, 0, template, cookie.name, cookie.value, vars[0], vars[1]) + 1;
        string = malloc(string_length);
        if (string == NULL) return;
        snprintf(string, string_length, template, cookie.name, cookie.value, vars[0], vars[1]);
        break;
    }
    case 5:
    {
        size_t string_length = snprintf(NULL, 0, template, cookie.name, cookie.value, vars[0], vars[1], vars[2]) + 1;
        string = malloc(string_length);
        if (string == NULL) return;
        snprintf(string, string_length, template, cookie.name, cookie.value, vars[0], vars[1], vars[2]);
        break;
    }
    case 6:
    {
        size_t string_length = snprintf(NULL, 0, template, cookie.name, cookie.value, vars[0], vars[1], vars[2], vars[3]) + 1;
        string = malloc(string_length);
        if (string == NULL) return;
        snprintf(string, string_length, template, cookie.name, cookie.value, vars[0], vars[1], vars[2], vars[3]);
        break;
    }
    default:
        return;
    }

    if (string == NULL) return;

    response->header_add(response, "Set-Cookie", string);

    free(string);
}

int http1response_redirect_is_external(const char* url) {
    if (strlen(url) < 8) return 0;

    if (url[0] == 'h'
        && url[1] == 't'
        && url[2] == 't'
        && url[3] == 'p'
        && url[4] == ':'
        && url[5] == '/'
        && url[6] == '/'
        ) return 1;

    if (url[0] == 'h'
        && url[1] == 't'
        && url[2] == 't'
        && url[3] == 'p'
        && url[4] == 's'
        && url[5] == ':'
        && url[6] == '/'
        && url[7] == '/'
        ) return 1;

    return 0;
}

int http1response_has_payload(http1response_t* response) {
    return response->content_length > 0
        || response->transfer_encoding != TE_NONE;
}

void __http1response_payload_free(http1_payload_t* payload) {
    if (payload->file.fd <= 0) return;

    payload->file.close(&payload->file);
    unlink(payload->path);

    payload->pos = 0;

    free(payload->path);
    payload->path = NULL;

    free(payload->boundary);
    payload->boundary = NULL;

    http1_payloadpart_free(payload->part);
    payload->part = NULL;
}

void __http1response_init_payload(http1response_t* response) {
    response->payload_.pos = 0;
    response->payload_.file = file_alloc();
    response->payload_.path = NULL;
    response->payload_.part = NULL;
    response->payload_.boundary = NULL;
    response->payload_.type = NONE;

    response->payload = __http1response_payload;
    response->payload_file = __http1response_payload_file;
    response->payload_json = __http1response_payload_json;
}

void __http1response_payload_parse_plain(http1response_t* response) {
    http1_payloadpart_t* part = http1_payloadpart_create();
    if (part == NULL) return;

    part->size = response->payload_.file.size;

    response->payload_.type = PLAIN;
    response->payload_.part = part;
}

char* __http1response_payload(http1response_t* response) {
    __http1response_payload_parse_plain(response);

    http1_payloadpart_t* part = response->payload_.part;
    if (part == NULL) return NULL;

    char* buffer = malloc(part->size + 1);
    if (buffer == NULL) return NULL;

    lseek(response->payload_.file.fd, part->offset, SEEK_SET);
    int r = read(response->payload_.file.fd, buffer, part->size);
    lseek(response->payload_.file.fd, 0, SEEK_SET);

    buffer[part->size] = 0;

    if (r < 0) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

file_content_t __http1response_payload_file(http1response_t* response) {
    __http1response_payload_parse_plain(response);

    file_content_t file_content = file_content_create(0, NULL, 0, 0);
    http1_payloadpart_t* part = response->payload_.part;
    if (part == NULL) return file_content;

    char* filename = NULL;
    http1_payloadfield_t* pfield = part->field;

    while (pfield) {
        if (pfield->key && strcmp(pfield->key, "filename") == 0) {
            filename = pfield->value;
            break;
        }
        pfield = pfield->next;
    }

    const char* field = NULL;
    file_content.ok = !(field != NULL && filename == NULL);
    file_content.fd = response->payload_.file.fd;
    file_content.offset = part->offset;
    file_content.size = part->size;
    file_content.set_filename(&file_content, filename);

    return file_content;
}

jsondoc_t* __http1response_payload_json(http1response_t* response) {
    char* payload = __http1response_payload(response);
    if (payload == NULL) return NULL;

    jsondoc_t* document = json_init();
    if (document == NULL) goto failed;
    if (!json_parse(document, payload)) goto failed;

    failed:

    free(payload);

    return document;
}
