#ifndef __HTTP1RESPONSE__
#define __HTTP1RESPONSE__

#include "connection.h"
#include "http1common.h"
#include "json.h"
#include "response.h"

typedef struct http1response_head {
    size_t size;
    char* data;
} http1response_head_t, http1response_string_t;

typedef struct {
    const char* name;
    const char* value;
    int minutes;
    const char* path;
    const char* domain;
    int secure;
    int http_only;
    const char* same_site;
} cookie_t;

typedef struct http1response {
    response_t base;
    http1_version_e version;
    http1_payload_t payload_;

    int status_code;
    int head_writed;
    http1_trunsfer_encoding_t transfer_encoding;
    http1_content_encoding_t content_encoding;
    size_t content_length;

    http1_body_t body;
    file_t file_;
    size_t file_pos;
    http1_header_t* header_;
    http1_header_t* last_header;
    http1_ranges_t* ranges;

    void* parser;
    connection_t* connection;
    z_stream defstream;

    void(*data)(struct http1response*, const char*);
    void(*datan)(struct http1response*, const char*, size_t);
    void(*def)(struct http1response*, int);
    void(*redirect)(struct http1response*, const char*, int);
    http1_header_t*(*header)(struct http1response*, const char*);
    int(*header_add)(struct http1response*, const char*, const char*);
    int(*headern_add)(struct http1response*, const char*, size_t, const char*, size_t);
    int(*headeru_add)(struct http1response*, const char*, size_t, const char*, size_t);
    int(*header_add_content_length)(struct http1response*, size_t);
    int(*header_remove)(struct http1response*, const char*);
    int(*headern_remove)(struct http1response*, const char*, size_t);
    int(*file)(struct http1response*, const char*);
    int(*filen)(struct http1response*, const char*, size_t);
    int(*filef)(struct http1response*, const char*, const char*, ...);
    int(*deflate)(struct http1response*, const char*, size_t, int, ssize_t(*)(connection_t*, const char*, size_t, int));
    int(*inflate)(struct http1response*, const char*, size_t);
    void(*cookie_add)(struct http1response*, cookie_t);

    char*(*payload)(struct http1response*);
    file_content_t(*payload_file)(struct http1response*);
    jsondoc_t*(*payload_json)(struct http1response*);
} http1response_t;

http1response_t* http1response_create(connection_t*);

void http1response_default(http1response_t*, int);

http1response_head_t http1response_create_head(http1response_t*);

void http1response_redirect(http1response_t*, const char*, int);

int http1response_data_append(char*, size_t*, const char*, size_t);

http1_ranges_t* http1response_init_ranges();

void http1response_free_ranges(http1_ranges_t*);

int http1response_redirect_is_external(const char* url);

const char* http1response_status_string(int);

char* http1response_payload(http1response_t*);
file_content_t http1response_payload_file(http1response_t*);
jsondoc_t* http1response_payload_json(http1response_t*);
int http1response_has_payload(http1response_t*);

#endif
