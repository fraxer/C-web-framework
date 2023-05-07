#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include "http1request.h"

void http1request_reset(http1request_t*);
http1_header_t* http1request_header(http1request_t*, const char*);
http1_header_t* http1request_headern(http1request_t*, const char*, size_t);
db_t* http1request_database_list(http1request_t*);
void http1request_payload_free(http1_payload_t*);
int http1request_file_save(http1_payloadfile_t*, const char*, const char*);
char* http1request_file_read(http1_payloadfile_t*, size_t, size_t);

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
    request->payload_.fd = 0;
    request->payload_.path = NULL;
    request->payload_.part = NULL;
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
    request->connection = connection;
    request->header = http1request_header;
    request->headern = http1request_headern;
    request->database_list = http1request_database_list;
    request->base.reset = (void(*)(void*))http1request_reset;
    request->base.free = (void(*)(void*))http1request_free;

    request->payload = http1request_payload;
    request->payloadf = http1request_payloadf;
    request->payload_urlencoded = http1request_payload_urlencoded;
    request->payload_file = http1request_payload_file;
    request->payload_filef = http1request_payload_filef;
    request->payload_json = http1request_payload_json;
    request->payload_jsonf = http1request_payload_jsonf;

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
}

char* http1request_payload(http1request_t* request) {
    return http1request_payloadf(request, NULL);
}

char* http1request_payloadf(http1request_t* request, const char* field) {
    http1_payloadpart_t* part = request->payload_.part;

    if (part == NULL) return NULL;

    while (field != NULL && part) {
        if (strcmp(part->field, field) == 0)
            break;

        part = part->next;
    }

    char* buffer = malloc(part->size + 1);
    if (buffer == NULL) return NULL;

    lseek(request->payload_.fd, part->offset, SEEK_SET);
    int r = read(request->payload_.fd, buffer, part->size);
    lseek(request->payload_.fd, 0, SEEK_SET);

    if (r <= 0) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

char* http1request_payload_urlencoded(http1request_t* request, const char* field) {
    http1_payloadpart_t* part = request->payload_.part;

    if (part == NULL) return NULL;

    while (field != NULL && part) {
        if (strcmp(part->field, field) == 0) {
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

    if (r <= 0) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

http1_payloadfile_t* http1request_payload_file(http1request_t* request) {
    return http1request_payload_filef(request, NULL);
}

http1_payloadfile_t* http1request_payload_filef(http1request_t* request, const char* field) {
    http1_payloadpart_t* part = request->payload_.part;

    if (part == NULL) return NULL;

    while (field != NULL && part) {
        if (strcmp(part->field, field) == 0)
            break;

        part = part->next;
    }

    http1_payloadfile_t* file = malloc(sizeof *file);
    if (file == NULL) return NULL;

    file->payload_fd = request->payload_.fd;
    file->offset = part->offset;
    file->size = part->size;
    file->rootdir = request->connection->server->root;
    file->name = NULL;
    file->save = http1request_file_save;
    file->read = http1request_file_read;

    return file;
}

jsmntok_t* http1request_payload_json(http1request_t* request) {
    return http1request_payload_jsonf(request, NULL);
}

jsmntok_t* http1request_payload_jsonf(http1request_t* request, const char* field) {
    http1_payloadpart_t* part = request->payload_.part;

    if (part == NULL) return NULL;

    while (field != NULL && part) {
        if (strcmp(part->field, field) == 0)
            break;

        part = part->next;
    }

    char* buffer = malloc(part->size + 1);
    if (buffer == NULL) return NULL;

    lseek(request->payload_.fd, part->offset, SEEK_SET);
    int r = read(request->payload_.fd, buffer, part->size);
    lseek(request->payload_.fd, 0, SEEK_SET);

    if (r <= 0) {
        free(buffer);
        return NULL;
    }

    jsmn_parser_t parser;

    if (jsmn_init(&parser, buffer) == -1) {
        free(buffer);
        return NULL;
    }
    if (jsmn_parse(&parser) < 0) {
        free(buffer);
        return NULL;
    }

    return jsmn_get_root_token(&parser);
}

int http1request_file_save(http1_payloadfile_t* file, const char* dir, const char* filename) {
    if (dir == NULL || filename == NULL) return 0;

    char path[PATH_MAX] = {0};
    size_t rootdir_length = strlen(file->rootdir);
    size_t dir_length = strlen(dir);
    
    strcpy(path, file->rootdir);
    if (file->rootdir[rootdir_length - 1] != '/') strcat(path, "/");
    strcat(path, dir);
    if (dir[dir_length - 1] != '/') strcat(path, "/");
    strcat(path, filename);

    int fd = open(path, O_CREAT | O_TRUNC);
    if (fd < 0) return 0;

    off_t offset = file->offset;
    sendfile(fd, file->payload_fd, &offset, file->size);
    close(fd);

    return 1;
}

char* http1request_file_read(http1_payloadfile_t* file, size_t offset, size_t size) {
    char* buffer = malloc(file->size + 1);
    if (buffer == NULL) return NULL;

    lseek(file->payload_fd, offset, SEEK_SET);
    int r = read(file->payload_fd, buffer, size);
    lseek(file->payload_fd, 0, SEEK_SET);

    if (r <= 0) {
        free(buffer);
        return NULL;
    }

    return buffer;
}