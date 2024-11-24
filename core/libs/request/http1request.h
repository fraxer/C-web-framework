#ifndef __HTTP1REQUEST__
#define __HTTP1REQUEST__

#include "route.h"
#include "connection.h"
#include "http1common.h"
#include "json.h"
#include "request.h"

typedef struct http1request_head {
    size_t size;
    char* data;
} http1request_head_t;

typedef struct http1request {
    request_t base;
    route_methods_e method;
    http1_version_e version;
    http1_payload_t payload_;
    http1_trunsfer_encoding_t transfer_encoding;
    http1_content_encoding_t content_encoding;
    void* parser;

    size_t uri_length;
    size_t path_length;
    size_t ext_length;

    const char* uri;
    const char* path;
    const char* ext;

    http1_query_t* query_;
    http1_query_t* last_query;
    http1_header_t* header_;
    http1_header_t* last_header;
    http1_cookie_t* cookie_;

    connection_t* connection;

    const char*(*query)(struct http1request*, const char*);

    http1_header_t*(*header)(struct http1request*, const char*);
    http1_header_t*(*headern)(struct http1request*, const char*, size_t);
    int(*header_add)(struct http1request*, const char*, const char*);
    int(*headern_add)(struct http1request*, const char*, size_t, const char*, size_t);
    int(*header_del)(struct http1request*, const char*);

    const char*(*cookie)(struct http1request*, const char*);

    char*(*payload)(struct http1request*);
    char*(*payloadf)(struct http1request*, const char*);
    file_content_t(*payload_file)(struct http1request*);
    file_content_t(*payload_filef)(struct http1request*, const char*);
    jsondoc_t*(*payload_json)(struct http1request*);
    jsondoc_t*(*payload_jsonf)(struct http1request*, const char*);

    int(*append_urlencoded)(struct http1request*, const char*, const char*);
    int(*append_formdata_raw)(struct http1request*, const char*, const char*, const char*);
    int(*append_formdata_text)(struct http1request*, const char*, const char*);
    int(*append_formdata_json)(struct http1request*, const char*, jsondoc_t*);
    int(*append_formdata_filepath)(struct http1request*, const char*, const char*);
    int(*append_formdata_file)(struct http1request*, const char*, file_t*);
    int(*append_formdata_file_content)(struct http1request*, const char*, file_content_t*);

    int(*set_payload_raw)(struct http1request*, const char*, const size_t, const char*);
    int(*set_payload_text)(struct http1request*, const char*);
    int(*set_payload_json)(struct http1request*, jsondoc_t*);
    int(*set_payload_filepath)(struct http1request*, const char*);
    int(*set_payload_file)(struct http1request*, const file_t*);
    int(*set_payload_file_content)(struct http1request*, const file_content_t*);
} http1request_t;

http1request_t* http1request_create(connection_t*);
char* http1request_payload(http1request_t*);
char* http1request_payloadf(http1request_t*, const char*);
file_content_t http1request_payload_file(http1request_t*);
file_content_t http1request_payload_filef(http1request_t*, const char*);
jsondoc_t* http1request_payload_json(http1request_t*);
jsondoc_t* http1request_payload_jsonf(http1request_t*, const char*);
int http1request_has_payload(http1request_t*);
int http1parser_set_uri(http1request_t*, const char*, size_t);
void http1parser_append_query(http1request_t*, http1_query_t*);
http1request_head_t http1request_create_head(http1request_t*);

#endif
