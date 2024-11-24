#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <errno.h>

#include "file.h"
#include "helpers.h"
#include "appconfig.h"
#include "mimetype.h"
#include "urlencodedparser.h"
#include "formdataparser.h"
#include "multipartparser.h"
#include "log.h"
#include "http1request.h"
#include "http1requestparser.h"

static const size_t boundary_size = 30;

int http1request_init_parser(http1request_t*);
void http1request_init_payload(http1request_t*);
void http1request_reset(http1request_t*);
const char* http1request_query(http1request_t*, const char*);
http1_header_t* http1request_header(http1request_t*, const char*);
http1_header_t* http1request_headern(http1request_t*, const char*, size_t);
int http1request_header_add(http1request_t*, const char*, const char*);
int http1request_headern_add(http1request_t*, const char*, size_t, const char*, size_t);
int http1request_header_del(http1request_t*, const char*);
const char* http1request_cookie(http1request_t*, const char*);
void http1request_payload_free(http1_payload_t*);
void http1request_payload_parse(http1request_t*);
http1_payloadpart_t* http1request_multipart_part(http1request_t*, const char*);
http1_payloadpart_t* http1request_urlencoded_part(http1request_t*, const char*);
int http1request_append_urlencoded(http1request_t*, const char*, const char*);
int http1request_append_formdata_raw(http1request_t*, const char*, const char*, const char*);
int http1request_append_formdata_text(http1request_t*, const char*, const char*);
int http1request_append_formdata_json(http1request_t*, const char*, jsondoc_t*);
int http1request_append_formdata_filepath(http1request_t*, const char*, const char*);
int http1request_append_formdata_file(http1request_t*, const char*, file_t*);
int http1request_append_formdata_file_content(http1request_t*, const char*, file_content_t*);
int http1request_set_payload_raw(http1request_t*, const char*, const size_t, const char*);
int http1request_set_payload_text(http1request_t*, const char*);
int http1request_set_payload_json(http1request_t*, jsondoc_t*);
int http1request_set_payload_filepath(http1request_t*, const char*);
int http1request_set_payload_file(http1request_t*, const file_t*);
int http1request_set_payload_file_content(http1request_t*, const file_content_t*);

int http1request_init_parser(http1request_t* request) {
    request->parser = malloc(sizeof(http1requestparser_t));
    if (request->parser == NULL) return 0;

    http1parser_init(request->parser);

    return 1;
}

void http1request_init_payload(http1request_t* request) {
    request->payload_.pos = 0;
    request->payload_.file = file_alloc();
    request->payload_.path = NULL;
    request->payload_.part = NULL;
    request->payload_.boundary = NULL;
    request->payload_.type = NONE;

    request->payload = http1request_payload;
    request->payloadf = http1request_payloadf;
    request->payload_file = http1request_payload_file;
    request->payload_filef = http1request_payload_filef;
    request->payload_json = http1request_payload_json;
    request->payload_jsonf = http1request_payload_jsonf;
}

http1request_t* http1request_alloc() {
    return (http1request_t*)malloc(sizeof(http1request_t));
}

void http1request_free(void* arg) {
    http1request_t* request = (http1request_t*)arg;

    http1request_reset(request);
    http1parser_free(request->parser);

    free(request);

    request = NULL;
}

http1request_t* http1request_create(connection_t* connection) {
    http1request_t* request = http1request_alloc();
    if (request == NULL) return NULL;

    request->method = ROUTE_NONE;
    request->version = HTTP1_VER_NONE;
    request->transfer_encoding = TE_NONE;
    request->content_encoding = CE_NONE;
    request->uri_length = 0;
    request->path_length = 0;
    request->ext_length = 0;
    request->uri = NULL;
    request->path = NULL;
    request->ext = NULL;
    request->ext = NULL;
    request->query_ = NULL;
    request->last_query = NULL;
    request->header_ = NULL;
    request->last_header = NULL;
    request->cookie_ = NULL;
    request->connection = connection;
    request->parser = NULL;
    request->query = http1request_query;
    request->header = http1request_header;
    request->headern = http1request_headern;
    request->header_add = http1request_header_add;
    request->headern_add = http1request_headern_add;
    request->cookie = http1request_cookie;
    request->header_del = http1request_header_del;
    request->append_urlencoded = http1request_append_urlencoded;
    request->append_formdata_raw = http1request_append_formdata_raw;
    request->append_formdata_text = http1request_append_formdata_text;
    request->append_formdata_json = http1request_append_formdata_json;
    request->append_formdata_filepath = http1request_append_formdata_filepath;
    request->append_formdata_file = http1request_append_formdata_file;
    request->append_formdata_file_content = http1request_append_formdata_file_content;
    request->set_payload_raw = http1request_set_payload_raw;
    request->set_payload_text = http1request_set_payload_text;
    request->set_payload_json = http1request_set_payload_json;
    request->set_payload_filepath = http1request_set_payload_filepath;
    request->set_payload_file = http1request_set_payload_file;
    request->set_payload_file_content = http1request_set_payload_file_content;
    request->base.reset = (void(*)(void*))http1request_reset;
    request->base.free = (void(*)(void*))http1request_free;

    http1request_init_payload(request);

    if (!http1request_init_parser(request)) {
        free(request);
        return NULL;
    }

    return request;
}

void http1request_reset(http1request_t* request) {
    request->method = ROUTE_NONE;
    request->version = HTTP1_VER_NONE;
    request->uri_length = 0;
    request->path_length = 0;
    request->ext_length = 0;

    if (request->uri) free((void*)request->uri);
    request->uri = NULL;

    if (request->path) free((void*)request->path);
    request->path = NULL;

    if (request->ext) free((void*)request->ext);
    request->ext = NULL;

    http1request_payload_free(&request->payload_);

    http1_queries_free(request->query_);
    request->query_ = NULL;
    request->last_query = NULL;

    http1_headers_free(request->header_);
    request->header_ = NULL;
    request->last_header = NULL;

    http1_cookie_free(request->cookie_);
    request->cookie_ = NULL;

    http1parser_reset(request->parser);
}

const char* http1request_query(http1request_t* request, const char* key) {
    if (key == NULL) return NULL;

    http1_query_t* query = request->query_;

    while (query) {
        if (strcmp(key, query->key) == 0)
            return query->value;

        query = query->next;
    }

    return NULL;
}

http1_header_t* http1request_header(http1request_t* request, const char* key) {
    return http1request_headern(request, key, strlen(key));
}

http1_header_t* http1request_headern(http1request_t* request, const char* key, size_t key_length) {
    http1_header_t* header = request->header_;

    while (header) {
        for (size_t i = 0, j = 0; i < header->key_length && j < key_length; i++, j++) {
            if (tolower(header->key[i]) != tolower(key[j])) goto next;
        }

        return header;

        next:

        header = header->next;
    }

    return NULL;
}

const char* http1request_cookie(http1request_t* request, const char* key) {
    http1_cookie_t* cookie = request->cookie_;
    size_t key_length = strlen(key);

    while (cookie) {
        for (size_t i = 0, j = 0; i < cookie->key_length && j < key_length; i++, j++) {
            if (tolower(cookie->key[i]) != tolower(key[j])) goto next;
        }

        return cookie->value;

        next:

        cookie = cookie->next;
    }

    return NULL;
}

void http1request_payload_free(http1_payload_t* payload) {
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

char* http1request_payload(http1request_t* request) {
    return http1request_payloadf(request, NULL);
}

char* http1request_payloadf(http1request_t* request, const char* field) {
    http1request_payload_parse(request);

    http1_payloadpart_t* part = NULL;

    if (request->payload_.type == PLAIN || request->payload_.type == MULTIPART)
        part = http1request_multipart_part(request, field);
    else if (request->payload_.type == URLENCODED)
        part = http1request_urlencoded_part(request, field);
    else
        return NULL;

    if (part == NULL) return NULL;

    char* buffer = malloc(part->size + 1);
    if (buffer == NULL) return NULL;

    lseek(request->payload_.file.fd, part->offset, SEEK_SET);
    int r = read(request->payload_.file.fd, buffer, part->size);
    lseek(request->payload_.file.fd, 0, SEEK_SET);

    buffer[part->size] = 0;

    if (r < 0) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

http1_payloadpart_t* http1request_multipart_part(http1request_t* request, const char* field) {
    http1_payloadpart_t* part = request->payload_.part;
    if (part == NULL) return NULL;

    while (field != NULL && part) {
        http1_payloadfield_t* pfield = part->field;
        while (pfield) {
            if (pfield->key != NULL && strcmp(pfield->key, "name") == 0 && strcmp(pfield->value, field) == 0)
                return part;

            pfield = pfield->next;
        }
        part = part->next;
    }

    return part;
}

http1_payloadpart_t* http1request_urlencoded_part(http1request_t* request, const char* field) {
    http1_payloadpart_t* part = request->payload_.part;
    if (part == NULL) return NULL;
    if (field == NULL) return part->next;

    while (field != NULL && part) {
        if (part->field && strcmp(part->field->value, field) == 0)
            return part->next;

        part = part->next;
    }

    return NULL;
}

file_content_t http1request_payload_file(http1request_t* request) {
    return http1request_payload_filef(request, NULL);
}

file_content_t http1request_payload_filef(http1request_t* request, const char* field) {
    http1request_payload_parse(request);

    file_content_t file_content = file_content_create(0, NULL, 0, 0);
    http1_payloadpart_t* part = http1request_multipart_part(request, field);
    if (part == NULL) return file_content;

    const char* filename = NULL;
    http1_payloadfield_t* pfield = part->field;
    while (pfield) {
        if (pfield->key && strcmp(pfield->key, "filename") == 0) {
            filename = pfield->value;
            break;
        }
        pfield = pfield->next;
    }

    file_content.ok = !(field != NULL && filename == NULL);
    file_content.fd = request->payload_.file.fd;
    file_content.offset = part->offset;
    file_content.size = part->size;
    file_content.set_filename(&file_content, filename);

    return file_content;
}

jsondoc_t* http1request_payload_json(http1request_t* request) {
    return http1request_payload_jsonf(request, NULL);
}

jsondoc_t* http1request_payload_jsonf(http1request_t* request, const char* field) {
    char* payload = http1request_payloadf(request, field);
    if (payload == NULL) return NULL;

    jsondoc_t* document = json_init();
    if (document == NULL) goto failed;
    if (!json_parse(document, payload)) goto failed;

    failed:

    free(payload);

    return document;
}

int http1request_has_payload(http1request_t* request) {
    switch (request->method) {
    case ROUTE_POST:
    case ROUTE_PUT:
    case ROUTE_PATCH:
        return 1;
    default:
        break;
    }

    return 0;
}

void http1request_payload_parse_multipart(http1request_t* request, const char* header_value) {
    size_t payload_size = strlen(header_value);
    formdataparser_t fdparser;
    formdataparser_init(&fdparser, payload_size);
    formdataparser_parse(&fdparser, header_value, payload_size);

    formdatalocation_t boundary_field = formdataparser_field(&fdparser, "boundary");

    formdataparser_free(&fdparser);

    if (!boundary_field.ok) return;

    char* boundary = malloc(boundary_field.size + 1);
    if (boundary == NULL) return;
    if (boundary_field.size == 0) {
        free(boundary);
        return;
    }
    strncpy(boundary, &header_value[boundary_field.offset], boundary_field.size);
    boundary[boundary_field.size] = 0;

    multipartparser_t mparser;
    multipartparser_init(&mparser, request->payload_.file.fd, boundary);

    size_t buffer_size = env()->main.buffer_size;
    char* buffer = malloc(buffer_size);
    if (buffer == NULL) {
        free(boundary);
        return;
    }

    lseek(request->payload_.file.fd, 0, SEEK_SET);

    while (1) {
        int r = read(request->payload_.file.fd, buffer, buffer_size);
        if (r <= 0) break;

        multipartparser_parse(&mparser, buffer, r);
    }

    free(boundary);
    free(buffer);

    lseek(request->payload_.file.fd, 0, SEEK_SET);

    request->payload_.type = MULTIPART;
    request->payload_.part = multipartparser_part(&mparser);
}

void http1request_payload_parse_urlencoded(http1request_t* request) {
    size_t buffer_size = env()->main.buffer_size;
    char* buffer = malloc(buffer_size);
    if (buffer == NULL) return;

    off_t payload_size = request->payload_.file.size;
    urlencodedparser_t parser;
    urlencodedparser_init(&parser, request->payload_.file.fd, payload_size);

    while (1) {
        int r = read(request->payload_.file.fd, buffer, buffer_size);
        if (r <= 0) break;

        urlencodedparser_parse(&parser, buffer, r);
    }

    free(buffer);

    lseek(request->payload_.file.fd, 0, SEEK_SET);

    request->payload_.type = URLENCODED;
    request->payload_.part = urlencodedparser_part(&parser);
}

void http1request_payload_parse_plain(http1request_t* request) {
    http1_payloadpart_t* part = http1_payloadpart_create();
    if (part == NULL) return;

    part->size = request->payload_.file.size;

    request->payload_.type = PLAIN;
    request->payload_.part = part;
}

void http1request_payload_parse(http1request_t* request) {
    if (request->payload_.file.fd == 0) return;
    if (request->payload_.part != NULL) return;

    http1_header_t* header = request->header_;

    while (header) {
        if (cmpstr_lower(header->key, "content-type"))
            break;

        header = header->next;
    }

    if (header == NULL) {
        http1request_payload_parse_plain(request);
        return;
    }

    if (cmpsubstr_lower(header->value, "multipart/form-data"))
        http1request_payload_parse_multipart(request, header->value);
    else if (cmpstr_lower(header->value, "application/x-www-form-urlencoded"))
        http1request_payload_parse_urlencoded(request);
    else
        http1request_payload_parse_plain(request);
}

int http1request_header_add(http1request_t* request, const char* key, const char* value) {
    return http1request_headern_add(request, key, strlen(key), value, strlen(value));
}

int http1request_headern_add(http1request_t* request, const char* key, size_t key_length, const char* value, size_t value_length) {
    http1_header_t* header = http1_header_create(key, key_length, value, value_length);
    if (header == NULL) return -1;
    if (header->key == NULL || header->value == NULL) {
        http1_header_free(header);
        return -1;
    }

    if (request->header_ == NULL)
        request->header_ = header;

    if (request->last_header != NULL)
        request->last_header->next = header;

    request->last_header = header;

    return 0;
}

int http1request_header_del(http1request_t* request, const char* key) {
    request->header_ = http1_header_delete(request->header_, key);

    http1_header_t* header = request->header_;
    while (header) {
        if (header->next == NULL) {
            request->last_header = header;
            break;
        }
        header = header->next;
    }

    return 0;
}

const char* http1request_method_string(route_methods_e method) {
    switch (method) {
    case ROUTE_NONE: return 0;
    case ROUTE_GET: return "GET";
    case ROUTE_POST: return "POST";
    case ROUTE_PUT: return "PUT";
    case ROUTE_DELETE: return "DELETE";
    case ROUTE_OPTIONS: return "OPTIONS";
    case ROUTE_PATCH: return "PATCH";
    case ROUTE_HEAD: return "HEAD";
    }

    return 0;
}

size_t http1request_method_length(route_methods_e method) {
    switch (method) {
    case ROUTE_NONE: return 0;
    case ROUTE_GET: return 3;
    case ROUTE_POST: return 4;
    case ROUTE_PUT: return 3;
    case ROUTE_DELETE: return 6;
    case ROUTE_OPTIONS: return 7;
    case ROUTE_PATCH: return 5;
    case ROUTE_HEAD: return 4;
    }

    return 0;
}

size_t http1request_head_size(http1request_t* request) {
    size_t size = http1request_method_length(request->method);
    if (size == 0) return 0;

    size += 1; // space
    size += request->uri_length;
    size += 11; // " HTTP/X.X\r\n"

    http1_header_t* header = request->header_;
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

int http1request_data_append(char* data, size_t* pos, const char* string, size_t length) {
    memcpy(&data[*pos], string, length);

    *pos += length;

    return 0;
}

http1request_head_t http1request_create_head(http1request_t* request) {
    http1request_head_t head = {
        .size = http1request_head_size(request),
        .data = malloc(head.size)
    };

    if (head.size == 0) return head;

    size_t pos = 0;
    if (http1request_data_append(head.data, &pos, http1request_method_string(request->method), http1request_method_length(request->method)) == -1) return head;
    if (http1request_data_append(head.data, &pos, " ", 1) == -1) return head;
    if (http1request_data_append(head.data, &pos, request->uri, request->uri_length) == -1) return head;
    if (http1request_data_append(head.data, &pos, " HTTP/1.1\r\n", 11) == -1) return head;

    http1_header_t* header = request->header_;
    while (header) {
        if (http1request_data_append(head.data, &pos, header->key, header->key_length) == -1) return head;
        if (http1request_data_append(head.data, &pos, ": ", 2) == -1) return head;
        if (http1request_data_append(head.data, &pos, header->value, header->value_length) == -1) return head;
        if (http1request_data_append(head.data, &pos, "\r\n", 2) == -1) return head;

        header = header->next;
    }

    if (http1request_data_append(head.data, &pos, "\r\n", 2) == -1) return head;

    return head;
}

int http1request_create_payload_file(http1_payload_t* payload) {
    payload->path = create_tmppath(env()->main.tmp);
    if (payload->path == NULL)
        return 0;

    payload->file.fd = mkstemp(payload->path);
    if (payload->file.fd == -1)
        return 0;

    return 1;
}

int http1request_append_urlencoded(http1request_t* request, const char* key, const char* value) {
    http1_payload_t* payload = &request->payload_;
    file_t* file = &payload->file;

    if (payload->type != NONE && payload->type != URLENCODED)
        return 0;

    if (file->fd <= 0) {
        if (!http1request_create_payload_file(&request->payload_))
            return 0;

        request->header_del(request, "Content-Type");
        request->headern_add(request, "Content-Type", 12, "application/x-www-form-urlencoded", 33);
    }
    else {
        if (!file->append_content(file, "&", 1))
            return 0;
    }

    if (!file->append_content(file, key, strlen(key)))
        return 0;
    if (!file->append_content(file, "=", 1))
        return 0;
    if (!file->append_content(file, value, strlen(value)))
        return 0;

    payload->type = URLENCODED;

    return 1;
}

int http1request_create_payload_boundary(http1_payload_t* payload, const size_t boundary_size) {
    payload->boundary = malloc(boundary_size + 1);
    if (payload->boundary == NULL)
        return 0;

    srand(time(NULL));
    const char* chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const size_t chars_length = 62;
    for (size_t i = 0; i < boundary_size; i++)
        payload->boundary[i] = *(chars + (rand() % chars_length));

    payload->boundary[boundary_size] = 0;

    return 1;
}

int http1request_insert_content_disposition_file_header_payload(http1_payload_t* payload, const char* key, const char* filename) {
    const char* template = "Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n";
    const size_t length = strlen(template) - 4 + strlen(key) + strlen(filename);
    char* string = malloc(length + 1);
    if (string == NULL)
        return 0;

    snprintf(string, length + 1, template, key, filename);
    const int result = payload->file.append_content(&payload->file, string, length);
    free(string);

    return result;
}

int http1request_insert_header_payload(http1_payload_t* payload, const char* template, const char* key) {
    const size_t length = strlen(template) - 2 + strlen(key);
    char* string = malloc(length + 1);
    if (string == NULL)
        return 0;

    snprintf(string, length + 1, template, key);
    const int result = payload->file.append_content(&payload->file, string, length);
    free(string);

    return result;
}

int http1request_add_header_multipart(http1request_t* request) {
    const char* multipart = "multipart/form-data; boundary=";
    const size_t multipart_length = strlen(multipart);
    const size_t length = multipart_length + strlen(request->payload_.boundary);
    char* string = malloc(length + 1);
    if (string == NULL)
        return 0;

    snprintf(string, length + 1, "%s%s", multipart, request->payload_.boundary);
    request->headern_add(request, "Content-Type", 12, string, length);
    free(string);

    return 1;
}

int http1request_append_formdata_raw(http1request_t* request, const char* key, const char* value, const char* mimetype) {
    http1_payload_t* payload = &request->payload_;
    file_t* file = &payload->file;

    if (payload->type != NONE && payload->type != MULTIPART)
        return 0;

    if (file->fd <= 0) {
        if (!http1request_create_payload_file(payload))
            return 0;
        
        if (!http1request_create_payload_boundary(payload, boundary_size))
            return 0;

        request->header_del(request, "Content-Type");
        if (!http1request_add_header_multipart(request))
            return 0;
    }
    else {
        file->size -= strlen(payload->boundary) + 6; // --boundary--\r\n
    }

    if (!file->append_content(file, "--", 2))
        return 0;
    if (!file->append_content(file, payload->boundary, boundary_size))
        return 0;
    if (!file->append_content(file, "\r\n", 2))
        return 0;

    if (!http1request_insert_header_payload(payload, "Content-Disposition: form-data; name=\"%s\"\r\n", key))
        return 0;
    if (!http1request_insert_header_payload(payload, "Content-Type: %s\r\n\r\n", mimetype))
        return 0;

    if (!file->append_content(file, value, strlen(value)))
        return 0;
    if (!file->append_content(file, "\r\n", 2))
        return 0;

    if (!file->append_content(file, "--", 2))
        return 0;
    if (!file->append_content(file, payload->boundary, boundary_size))
        return 0;
    if (!file->append_content(file, "--\r\n", 4))
        return 0;

    payload->type = MULTIPART;

    return 1;
}

int http1request_append_formdata_text(http1request_t* request, const char* key, const char* value) {
    return http1request_append_formdata_raw(request, key, value, "text/plain");
}

int http1request_append_formdata_json(http1request_t* request, const char* key, jsondoc_t* document) {
    return http1request_append_formdata_raw(request, key, json_stringify(document), "application/json");
}

int http1request_append_formdata_filepath(http1request_t* request, const char* key, const char* filepath) {
    file_t file = file_open(filepath, O_RDONLY);
    if (!file.ok) return 0;

    const int result = http1request_append_formdata_file(request, key, &file);
    file.close(&file);

    return result;
}

int http1request_append_formdata_file(http1request_t* request, const char* key, file_t* file) {
    file_content_t file_content = file_content_create(file->fd, file->name, 0, file->size);

    return http1request_append_formdata_file_content(request, key, &file_content);
}

int http1request_append_formdata_file_content(http1request_t* request, const char* key, file_content_t* file_content) {
    http1_payload_t* payload = &request->payload_;
    file_t* file = &payload->file;

    if (payload->type != NONE && payload->type != MULTIPART)
        return 0;
    if (!file_content->ok)
        return 0;
    if (file_content->fd <= 0)
        return 0;

    int result = 0;
    if (file->fd <= 0) {
        if (!http1request_create_payload_file(payload))
            goto failed;

        if (!http1request_create_payload_boundary(payload, boundary_size))
            goto failed;

        request->header_del(request, "Content-Type");
        if (!http1request_add_header_multipart(request))
            goto failed;
    }
    else {
        file->size -= strlen(payload->boundary) + 6; // --boundary--\r\n
    }

    if (!file->append_content(file, "--", 2))
        goto failed;
    if (!file->append_content(file, payload->boundary, boundary_size))
        goto failed;
    if (!file->append_content(file, "\r\n", 2))
        goto failed;
    if (!http1request_insert_content_disposition_file_header_payload(payload, key, file_content->filename))
        goto failed;

    const char* ext = file_extention(file_content->filename);
    const char* mimetype = mimetype_find_type(appconfig()->mimetype, ext);
    if (mimetype == NULL)
        mimetype = "text/plain";

    if (!http1request_insert_header_payload(payload, "Content-Type: %s\r\n\r\n", mimetype))
        goto failed;

    off_t offset = file_content->offset;
    lseek(file->fd, file->size, SEEK_SET);
    size_t sended = sendfile(file->fd, file_content->fd, &offset, file_content->size);
    lseek(file->fd, 0, SEEK_SET);
    file->size += sended;

    if (sended != file_content->size)
        goto failed;

    if (!file->append_content(file, "\r\n", 2))
        goto failed;
    if (!file->append_content(file, "--", 2))
        goto failed;
    if (!file->append_content(file, payload->boundary, boundary_size))
        goto failed;
    if (!file->append_content(file, "--\r\n", 4))
        goto failed;

    result = 1;

    payload->type = MULTIPART;

    failed:

    return result;
}

int http1request_set_payload_raw(http1request_t* request, const char* value, const size_t value_size, const char* mimetype) {
    http1_payload_t* payload = &request->payload_;
    file_t* file = &payload->file;

    if (payload->type != NONE && payload->type != PLAIN)
        return 0;

    if (file->fd <= 0) {
        if (!http1request_create_payload_file(payload))
            return 0;
    }
    else {
        if (!file->truncate(file, 0))
            return 0;
    }

    request->header_del(request, "Content-Type");
    request->header_add(request, "Content-Type", mimetype);

    if (!file->append_content(file, value, value_size))
        return 0;

    payload->type = PLAIN;

    return 1;
}

int http1request_set_payload_text(http1request_t* request, const char* value) {
    return http1request_set_payload_raw(request, value, strlen(value), "text/plain");
}

int http1request_set_payload_json(http1request_t* request, jsondoc_t* document) {
    return http1request_set_payload_raw(request, json_stringify(document), json_stringify_size(document), "application/json");
}

int http1request_set_payload_filepath(http1request_t* request, const char* filepath) {
    file_t file = file_open(filepath, O_RDONLY);
    if (!file.ok) return 0;

    const int result = http1request_set_payload_file(request, &file);
    file.close(&file);

    return result;
}

int http1request_set_payload_file(http1request_t* request, const file_t* file) {
    file_content_t file_content = file_content_create(file->fd, file->name, 0, file->size);

    return http1request_set_payload_file_content(request, &file_content);
}

int http1request_set_payload_file_content(http1request_t* request, const file_content_t* file_content) {
    http1_payload_t* payload = &request->payload_;
    file_t* file = &payload->file;

    if (payload->type != NONE && payload->type != PLAIN)
        return 0;
    if (!file_content->ok)
        return 0;
    if (file_content->fd <= 0)
        return 0;

    int result = 0;
    if (file->fd <= 0) {
        if (!http1request_create_payload_file(payload))
            goto failed;
    }
    else {
        if (!file->truncate(file, 0))
            goto failed;
    }

    const char* ext = file_extention(file_content->filename);
    const char* mimetype = mimetype_find_type(appconfig()->mimetype, ext);
    if (mimetype == NULL)
        mimetype = "text/plain";

    request->header_del(request, "Content-Type");
    request->header_add(request, "Content-Type", mimetype);

    off_t offset = file_content->offset;
    size_t filesize = file_content->size;

    lseek(file->fd, file->size, SEEK_SET);
    size_t sended = sendfile(file->fd, file_content->fd, &offset, filesize);
    lseek(file->fd, 0, SEEK_SET);
    file->size += sended;

    if (sended != filesize)
        goto failed;

    result = 1;

    payload->type = PLAIN;

    failed:

    return result;
}
