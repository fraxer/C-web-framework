#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <errno.h>

#include "helpers.h"
#include "urlencodedparser.h"
#include "formdataparser.h"
#include "multipartparser.h"
#include "log.h"
#include "http1request.h"

void http1request_init_payload(http1request_t*);
void http1request_reset(http1request_t*);
http1_header_t* http1request_header(http1request_t*, const char*);
http1_header_t* http1request_headern(http1request_t*, const char*, size_t);
const char* http1request_cookie(http1request_t*, const char*);
db_t* http1request_database_list(http1request_t*);
void http1request_payload_free(http1_payload_t*);
int http1request_file_save(http1_payloadfile_t*, const char*, const char*);
char* http1request_file_read(http1_payloadfile_t*);
void http1request_payload_parse(http1request_t*);

void http1request_init_payload(http1request_t* request) {
    request->payload_.fd = 0;
    request->payload_.path = NULL;
    request->payload_.part = NULL;
    request->payload_.boundary = NULL;

    request->payload = http1request_payload;
    request->payloadf = http1request_payloadf;
    request->payload_urlencoded = http1request_payload_urlencoded;
    request->payload_file = http1request_payload_file;
    request->payload_filef = http1request_payload_filef;
    request->payload_json = http1request_payload_json;
    request->payload_jsonf = http1request_payload_jsonf;
}

http1request_t* http1request_alloc() {
    return (http1request_t*)malloc(sizeof(http1request_t));
}

void http1request_header_free(http1_header_t* header) {
    while (header) {
        http1_header_t* next = header->next;

        http1_header_free(header);

        header = next;
    }
}

void http1request_query_free(http1_query_t* query) {
    while (query != NULL) {
        http1_query_t* next = query->next;

        http1_query_free(query);

        query = next;
    }
}

void http1request_free(void* arg) {
    http1request_t* request = (http1request_t*)arg;

    http1request_reset(request);

    free(request);

    request = NULL;
}

http1request_t* http1request_create(connection_t* connection) {
    http1request_t* request = http1request_alloc();

    if (request == NULL) return NULL;

    request->method = ROUTE_NONE;
    request->version = HTTP1_VER_NONE;
    request->uri_length = 0;
    request->path_length = 0;
    request->ext_length = 0;
    request->uri = NULL;
    request->path = NULL;
    request->ext = NULL;
    request->query = NULL;
    request->last_query = NULL;
    request->header_ = NULL;
    request->last_header = NULL;
    request->cookie_ = NULL;
    request->connection = connection;
    request->header = http1request_header;
    request->headern = http1request_headern;
    request->cookie = http1request_cookie;
    request->database_list = http1request_database_list;
    request->base.reset = (void(*)(void*))http1request_reset;
    request->base.free = (void(*)(void*))http1request_free;

    http1request_init_payload(request);

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

    http1request_query_free(request->query);
    request->query = NULL;
    request->last_query = NULL;

    http1request_header_free(request->header_);
    request->header_ = NULL;
    request->last_header = NULL;

    http1_cookie_free(request->cookie_);
    request->cookie_ = NULL;
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

db_t* http1request_database_list(http1request_t* request) {
    return request->connection->server->database;
}

void http1request_payload_free(http1_payload_t* payload) {
    if (payload->fd <= 0) return;

    close(payload->fd);
    unlink(payload->path);

    payload->fd = 0;

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

    char* buffer = NULL;
    http1_payloadpart_t* part = request->payload_.part;
    if (part == NULL) return NULL;

    while (field != NULL && part) {
        http1_payloadfield_t* pfield = part->field;
        while (pfield) {
            if (pfield->key != NULL && strcmp(pfield->key, "name") == 0 && strcmp(pfield->value, field) == 0) {
                goto next;
            }
            pfield = pfield->next;
        }
        part = part->next;
    }

    if (part == NULL) return NULL;

    next:

    buffer = malloc(part->size + 1);
    if (buffer == NULL) return NULL;

    lseek(request->payload_.fd, part->offset, SEEK_SET);
    int r = read(request->payload_.fd, buffer, part->size);
    lseek(request->payload_.fd, 0, SEEK_SET);

    buffer[part->size] = 0;

    if (r <= 0) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

char* http1request_payload_urlencoded(http1request_t* request, const char* field) {
    http1request_payload_parse(request);

    http1_payloadpart_t* part = request->payload_.part;

    if (part == NULL) return NULL;

    while (field != NULL && part) {
        if (part->field && strcmp(part->field->value, field) == 0) {
            part = part->next;
            break;
        }

        part = part->next;
    }

    if (part == NULL) return NULL;

    char* buffer = malloc(part->size + 1);
    if (buffer == NULL) return NULL;

    lseek(request->payload_.fd, part->offset, SEEK_SET);
    int r = read(request->payload_.fd, buffer, part->size);
    lseek(request->payload_.fd, 0, SEEK_SET);

    buffer[part->size] = 0;

    if (r < 0) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

http1_payloadfile_t http1request_payload_file(http1request_t* request) {
    return http1request_payload_filef(request, NULL);
}

http1_payloadfile_t http1request_payload_filef(http1request_t* request, const char* field) {
    http1request_payload_parse(request);

    http1_payloadfile_t file = {
        .ok = 0,
        .payload_fd = 0,
        .offset = 0,
        .size = 0,
        .rootdir = NULL,
        .name = NULL,
        .save = http1request_file_save,
        .read = http1request_file_read
    };

    http1_payloadpart_t* part = request->payload_.part;
    if (part == NULL) return file;

    while (field != NULL && part) {
        http1_payloadfield_t* pfield = part->field;
        while (pfield) {
            if (pfield->key != NULL && strcmp(pfield->key, "name") == 0 && strcmp(pfield->value, field) == 0) {
                goto next;
            }
            pfield = pfield->next;
        }
        part = part->next;
    }

    if (part == NULL) return file;

    char* filename = NULL;
    http1_payloadfield_t* pfield = NULL;

    next:

    pfield = part->field;

    while (pfield) {
        if (strcmp(pfield->key, "filename") == 0) {
            filename = pfield->value;
            break;
        }
        pfield = pfield->next;
    }

    file.ok = filename != NULL;
    file.payload_fd = request->payload_.fd;
    file.offset = part->offset;
    file.size = part->size;
    file.rootdir = request->connection->server->root;
    file.name = filename;

    return file;
}

jsondoc_t* http1request_payload_json(http1request_t* request) {
    return http1request_payload_jsonf(request, NULL);
}

jsondoc_t* http1request_payload_jsonf(http1request_t* request, const char* field) {
    char* payload = http1request_payloadf(request, field);
    if (payload == NULL) return NULL;

    jsondoc_t* document = json_init();
    if (!document) goto failed;
    if (json_parse(document, payload) < 0) goto failed;

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

int http1request_file_save(http1_payloadfile_t* file, const char* dir, const char* filename) {
    if (dir == NULL) return 0;
    if (filename == NULL) filename = file->name;
    size_t filename_length = strlen(filename);
    if (filename_length == 0) return 0;

    for (size_t i = 0; i < filename_length; i++)
        if (filename[i] == '/') return 0;

    char path[PATH_MAX] = {0};
    size_t rootdir_length = strlen(file->rootdir);
    size_t dir_length = strlen(dir);

    strcpy(path, file->rootdir);
    if (file->rootdir[rootdir_length - 1] != '/' && dir[0] != '/') strcat(path, "/");
    strcat(path, dir);
    if (dir[dir_length - 1] != '/' && filename[0] != '/') strcat(path, "/");
    strcat(path, filename);

    if (!helpers_mkdir(file->rootdir, dir)) return 0;

    int fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
    if (fd < 0) return 0;

    int result = 1;
    off_t offset = file->offset;
    if (sendfile(fd, file->payload_fd, &offset, file->size) == -1) {
        log_error("Request error: %s\n", strerror(errno));
        result = 0;
    }

    close(fd);

    return result;
}

char* http1request_file_read(http1_payloadfile_t* file) {
    char* buffer = malloc(file->size + 1);
    if (buffer == NULL) return NULL;

    lseek(file->payload_fd, file->offset, SEEK_SET);
    int r = read(file->payload_fd, buffer, file->size);
    lseek(file->payload_fd, 0, SEEK_SET);

    if (r <= 0) {
        free(buffer);
        return NULL;
    }

    buffer[file->size] = 0;

    return buffer;
}

int cmpstr(const char* a, const char* b) {
    size_t a_length = strlen(a);
    size_t b_length = strlen(b);

    for (size_t i = 0, j = 0; i < a_length && j < b_length; i++, j++) {
        if (tolower(a[i]) != tolower(b[j])) return 0;
    }

    return 1;
}

int cmpsubstr(const char* a, const char* b) {
    size_t a_length = strlen(a);
    size_t b_length = strlen(b);

    for (size_t i = 0, j = 0; i < a_length && j < b_length; i++, j++) {
        if (tolower(a[i]) != tolower(b[j])) return 0;
        if (i + 1 == a_length || j + 1 == b_length) return 1;
    }

    return 1;
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
    multipartparser_init(&mparser, request->payload_.fd, boundary);

    size_t buffer_size = request->connection->server->info->read_buffer;
    char* buffer = malloc(buffer_size);
    if (buffer == NULL) {
        free(boundary);
        return;
    }

    lseek(request->payload_.fd, 0, SEEK_SET);

    while (1) {
        int r = read(request->payload_.fd, buffer, buffer_size);
        if (r <= 0) break;

        multipartparser_parse(&mparser, buffer, r);
    }

    free(boundary);
    free(buffer);

    lseek(request->payload_.fd, 0, SEEK_SET);

    request->payload_.part = multipartparser_part(&mparser);
}

void http1request_payload_parse_urlencoded(http1request_t* request) {
    size_t buffer_size = request->connection->server->info->read_buffer;
    char* buffer = malloc(buffer_size);
    if (buffer == NULL) return;

    off_t payload_size = lseek(request->payload_.fd, 0, SEEK_END);
    lseek(request->payload_.fd, 0, SEEK_SET);

    urlencodedparser_t parser;
    urlencodedparser_init(&parser, request->payload_.fd, payload_size);

    while (1) {
        int r = read(request->payload_.fd, buffer, buffer_size);
        if (r <= 0) break;

        urlencodedparser_parse(&parser, buffer, r);
    }

    free(buffer);

    lseek(request->payload_.fd, 0, SEEK_SET);

    request->payload_.part = urlencodedparser_part(&parser);
}

void http1request_payload_parse_plain(http1request_t* request) {
    http1_payloadpart_t* part = http1_payloadpart_create();
    if (part == NULL) return;

    part->size = lseek(request->payload_.fd, 0, SEEK_END);
    lseek(request->payload_.fd, 0, SEEK_SET);

    request->payload_.part = part;
}

void http1request_payload_parse(http1request_t* request) {
    if (request->payload_.fd == 0) return;
    if (request->payload_.part != NULL) return;

    http1_header_t* header = request->header_;

    while (header) {
        if (cmpstr(header->key, "content-type"))
            break;

        header = header->next;
    }

    if (header == NULL) return;

    if (cmpsubstr(header->value, "multipart/form-data")) {
        http1request_payload_parse_multipart(request, header->value);
    }
    else if (cmpstr(header->value, "application/x-www-form-urlencoded")) {
        http1request_payload_parse_urlencoded(request);
    }
    else {
        http1request_payload_parse_plain(request);
    }
}
