#ifndef __HTTP1COMMON__
#define __HTTP1COMMON__

#include <stddef.h>

#include "file.h"

typedef struct http1_header {
    char* key;
    char* value;
    size_t key_length;
    size_t value_length;
    struct http1_header* next;
} http1_header_t, http1_cookie_t;

typedef enum http1_version {
    HTTP1_VER_NONE = 0,
    HTTP1_VER_1_0,
    HTTP1_VER_1_1
} http1_version_e;

typedef struct http1_query {
    const char* key;
    const char* value;
    struct http1_query* next;
} http1_query_t;

typedef struct http1_body {
    size_t size;
    size_t pos;
    char* data;
} http1_body_t;

typedef enum http1_content_encoding {
    CE_NONE = 0,
    CE_GZIP
} http1_content_encoding_t;

typedef enum http1_trunsfer_encoding {
    TE_NONE = 0,
    TE_CHUNKED,
    TE_GZIP
} http1_trunsfer_encoding_t;

typedef struct http1_ranges {
    ssize_t start;
    ssize_t end;
    ssize_t pos;
    struct http1_ranges* next;
} http1_ranges_t;

typedef struct http1_payloadfield {
    char* key;
    char* value;
    size_t key_length;
    size_t value_length;
    struct http1_payloadfield* next;
} http1_payloadfield_t;

typedef struct http1_payloadpart {
    size_t offset;
    size_t size;
    http1_payloadfield_t* field;
    http1_header_t* header;

    struct http1_payloadpart* next;
} http1_payloadpart_t;

typedef enum {
    NONE = 0,
    PLAIN,
    MULTIPART,
    URLENCODED
} http1_payload_type_e;

typedef struct http1_payload {
    size_t pos;
    file_t file;
    char* path;
    char* boundary;
    http1_payloadpart_t* part;
    http1_payload_type_e type;
} http1_payload_t;

typedef struct http1_urlendec {
    char* string;
    size_t length;
} http1_urlendec_t;

http1_header_t* http1_header_create(const char*, size_t, const char*, size_t);
void http1_header_free(http1_header_t*);
void http1_headers_free(http1_header_t*);
http1_header_t* http1_header_delete(http1_header_t*, const char*);

http1_query_t* http1_query_create(const char*, size_t, const char*, size_t);
void http1_query_free(http1_query_t*);
void http1_queries_free(http1_query_t*);

char* http1_set_field(const char*, size_t);

http1_payloadpart_t* http1_payloadpart_create();
void http1_payloadpart_free(http1_payloadpart_t*);
http1_payloadfield_t* http1_payloadfield_create();
void http1_payloadfield_free(http1_payloadfield_t*);

http1_urlendec_t http1_urlencode(const char*, size_t);
http1_urlendec_t http1_urldecode(const char*, size_t);

http1_cookie_t* http1_cookie_create();
void http1_cookie_free(http1_cookie_t*);

#endif
