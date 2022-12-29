#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "../mimetype/mimetype.h"
#include "websocketsresponse.h"
    #include <stdio.h>

void websocketsresponse_data(websocketsresponse_t*, const char*);
void websocketsresponse_datan(websocketsresponse_t*, const char*, size_t);
int websocketsresponse_file(websocketsresponse_t*, const char*);
int websocketsresponse_filen(websocketsresponse_t*, const char*, size_t);
size_t websocketsresponse_size(websocketsresponse_t*, size_t);
int websocketsresponse_prepare(websocketsresponse_t*, const char*, size_t);
const char* websocketsresponse_get_mimetype(const char*);
const char* websocketsresponse_get_extention(const char*, size_t);
void websocketsresponse_reset(websocketsresponse_t*);
int websocketsresponse_keepalive_enabled(websocketsresponse_t*);


websocketsresponse_t* websocketsresponse_alloc() {
    return (websocketsresponse_t*)malloc(sizeof(websocketsresponse_t));
}

void websocketsresponse_free(void* arg) {
    websocketsresponse_t* response = (websocketsresponse_t*)arg;

    websocketsresponse_reset(response);

    free(response);

    response = NULL;
}

websocketsresponse_t* websocketsresponse_create(connection_t* connection) {
    websocketsresponse_t* response = websocketsresponse_alloc();

    if (response == NULL) return NULL;

    response->body.data = NULL;
    response->body.pos = 0;
    response->body.size = 0;
    response->file_.fd = 0;
    response->file_.pos = 0;
    response->file_.size = 0;
    response->connection = connection;
    response->data = websocketsresponse_data;
    response->datan = websocketsresponse_datan;
    response->file = websocketsresponse_file;
    response->filen = websocketsresponse_filen;
    response->base.reset = (void(*)(void*))websocketsresponse_reset;
    response->base.free = (void(*)(void*))websocketsresponse_free;

    return response;
}

void websocketsresponse_reset(websocketsresponse_t* response) {
    // response->body.pos = 0;
    // response->body.size = 0;

    // if (response->file_.fd > 0) {
    //     lseek(response->file_.fd, 0, SEEK_SET);
    //     close(response->file_.fd);
    // }

    // response->file_.fd = 0;
    // response->file_.pos = 0;
    // response->file_.size = 0;

    // if (response->body.data) free(response->body.data);
    // response->body.data = NULL;
}

size_t websocketsresponse_size(websocketsresponse_t* response, size_t body_length) {
    size_t size = 0;

    size += body_length; // body length

    return size;
}

int websocketsresponse_data_append(char* data, size_t* pos, const char* string, size_t length) {
    memcpy(&data[*pos], string, length);

    *pos += length;
}

int websocketsresponse_prepare(websocketsresponse_t* response, const char* body, size_t length) {
    char* data = (char*)malloc(response->body.size);

    if (data == NULL) return -1;

    size_t pos = 0;

    if (body != NULL && websocketsresponse_data_append(data, &pos, body, length) == -1) return -1;

    response->body.data = data;

    return 0;
}

void websocketsresponse_data(websocketsresponse_t* response, const char* data) {
    websocketsresponse_datan(response, data, strlen(data));
}

void websocketsresponse_datan(websocketsresponse_t* response, const char* data, size_t length) {
    response->body.size = websocketsresponse_size(response, length);

    websocketsresponse_prepare(response, data, length);

    // printf("body: %s, %ld\n", response->body.data, response->body.size);
}

int websocketsresponse_file(websocketsresponse_t* response, const char* path) {
    return websocketsresponse_filen(response, path, strlen(path));
}

int websocketsresponse_filen(websocketsresponse_t* response, const char* path, size_t length) {
    {
        const char* root = response->connection->server->root;
        size_t root_length = response->connection->server->root_length;
        size_t fullpath_length = response->connection->server->root_length + length;
        size_t pos = 0;

        if (path[0] != '/') fullpath_length++;

        char fullpath[fullpath_length + 1];

        websocketsresponse_data_append(fullpath, &pos, root, root_length);

        if (path[0] != '/') websocketsresponse_data_append(fullpath, &pos, "/", 1);

        websocketsresponse_data_append(fullpath, &pos, path, length);

        fullpath[fullpath_length] = 0;

        struct stat stat_obj;

        stat(fullpath, &stat_obj);

        if (S_ISDIR(stat_obj.st_mode)) {
            websocketsresponse_default_response(response, 403);
            return -1;
        }

        if (!S_ISREG(stat_obj.st_mode)) {
            websocketsresponse_default_response(response, 404);
            return -1;
        }

        response->file_.fd = open(fullpath, O_RDONLY);
    }

    if (response->file_.fd == -1) return -1;

    response->file_.size = (size_t)lseek(response->file_.fd, 0, SEEK_END);

    lseek(response->file_.fd, 0, SEEK_SET);

    const char* ext = websocketsresponse_get_extention(path, length);

    const char* mimetype = websocketsresponse_get_mimetype(ext);

    response->body.size = websocketsresponse_size(response, 0);

    if (websocketsresponse_prepare(response, NULL, 0) == -1) return -1;

    // printf("body: %s, %ld\n", response->body.data, response->body.size);

    return 0;
}

const char* websocketsresponse_get_mimetype(const char* extension) {
    const char* mimetype = mimetype_find_type(extension);

    if (mimetype == NULL) return "text/plain";

    return mimetype;
}

const char* websocketsresponse_get_extention(const char* path, size_t length) {
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

void websocketsresponse_default_response(websocketsresponse_t* response, int status_code) {
    websocketsresponse_reset(response);

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

    websocketsresponse_datan(response, data, data_length);
}

int websocketsresponse_keepalive_enabled(websocketsresponse_t* response) {
    return response->connection->keepalive_enabled;
}