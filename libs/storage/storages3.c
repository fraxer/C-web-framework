 #include <openssl/evp.h>
 #include <openssl/hmac.h>
 
 #include "base64.h"
 #include "helpers.h"
 #include "mimetype.h"
 #include "storages3.h"
 #include "httpclient.h"

void __storages3_free(void* storage);
file_t __storages3_file_get(void* storage, const char* path);
int __storages3_file_put(void* storage, const file_t* file, const char* path);
int __storages3_file_content_put(void* storage, const file_content_t* file_content, const char* path);
int __storages3_file_remove(void* storage, const char* path);
int __storages3_file_exist(void* storage, const char* path);
int __storages3_create_signature(storages3_t* storage, const char* data, char* result);
char* __storages3_create_uri(storages3_t* storage, const char* path);
char* __storages3_create_url(storages3_t* storage, const char* uri);
char* __storages3_create_date();
char* __storages3_create_signdata(const char* method, const char* mimetype, const char* date, const char* uri);
char* __storages3_create_authtoken(storages3_t* storage, const char* method, const char* mimetype, const char* date, const char* uri);

storages3_t* storage_create_s3(const char* storage_name, const char* access_id, const char* access_secret, const char* protocol, const char* host, const char* port, const char* bucket) {
    storages3_t* storage = malloc(sizeof * storage);
    if (storage == NULL)
        return NULL;

    storage->base.type = STORAGE_TYPE_S3;
    storage->base.next = NULL;
    strcpy(storage->base.name, storage_name);
    strcpy(storage->access_id, access_id);
    strcpy(storage->access_secret, access_secret);
    strcpy(storage->protocol, protocol);
    strcpy(storage->host, host);
    strcpy(storage->port, port);
    strcpy(storage->bucket, bucket);

    storage->base.free = __storages3_free;
    storage->base.file_get = __storages3_file_get;
    storage->base.file_put = __storages3_file_put;
    storage->base.file_content_put = __storages3_file_content_put;
    storage->base.file_remove = __storages3_file_remove;
    storage->base.file_exist = __storages3_file_exist;

    return storage;
}

void __storages3_free(void* storage) {
    free(storage);
}

file_t __storages3_file_get(void* storage, const char* path) {
    storages3_t* s = storage;
    file_t result = file_alloc();

    const char* method = "GET";
    const char* mimetype = "";

    char* uri = __storages3_create_uri(s, path);
    if (uri == NULL) goto failed;

    char* url = __storages3_create_url(s, uri);
    if (url == NULL) goto failed;

    char* date = __storages3_create_date();
    if (date == NULL) goto failed;

    char* authorization = __storages3_create_authtoken(s, method, mimetype, date, uri);
    if (authorization == NULL) goto failed;

    const int timeout = 3;
    httpclient_t* client = httpclient_init(ROUTE_GET, url, timeout);
    if (client == NULL)
        goto failed;

    http1request_t* req = client->request;
    req->header_add(req, "Date", date);
    req->header_add(req, "Authorization", authorization);

    http1response_t* res = client->send(client);
    if (!res)
        goto failed;
    if (res->status_code != 200)
        goto failed;

    file_content_t file_content = res->payload_file(res);
    if (!file_content.ok)
        goto failed;

    const char* filename = basename((char*)path);
    if (strcmp(filename, "/") == 0) goto failed;
    if (strcmp(filename, ".") == 0) goto failed;
    if (strcmp(filename, "..") == 0) goto failed;

    file_content.set_filename(&file_content, filename);

    result = file_content.make_tmpfile(&file_content);

    failed:

    if (uri != NULL) free(uri);
    if (url != NULL) free(url);
    if (date != NULL) free(date);
    if (authorization != NULL) free(authorization);
    if (client != NULL) client->free(client);

    return result;
}

int __storages3_file_put(void* storage, const file_t* file, const char* path) {
    if (storage == NULL) return 0;
    if (file == NULL) return 0;
    if (path == NULL) return 0;

    file_content_t file_content = file_content_create(file->fd, file->name, 0, file->size);

    return __storages3_file_content_put(storage, &file_content, path);
}

int __storages3_file_content_put(void* storage, const file_content_t* file_content, const char* path) {
    storages3_t* s = storage;
    int result = 0;

    const char* method = "PUT";
    const char* ext = file_extention(file_content->filename);
    const char* mimetype = mimetype_find_type(ext);
    if (mimetype == NULL)
        mimetype = "text/plain";

    char* uri = __storages3_create_uri(s, path);
    if (uri == NULL) goto failed;

    char* url = __storages3_create_url(s, uri);
    if (url == NULL) goto failed;

    char* date = __storages3_create_date();
    if (date == NULL) goto failed;

    char* authorization = __storages3_create_authtoken(s, method, mimetype, date, uri);
    if (authorization == NULL) goto failed;

    const int timeout = 3;
    httpclient_t* client = httpclient_init(ROUTE_PUT, url, timeout);
    if (client == NULL)
        goto failed;

    http1request_t* req = client->request;

    char content_disposition[512];
    snprintf(content_disposition, sizeof(content_disposition), "attachment; filename=%s", file_content->filename);
    req->header_add(req, "Content-Disposition", content_disposition);
    req->header_add(req, "Date", date);
    req->header_add(req, "Authorization", authorization);
    req->header_add(req, "Content-Type", mimetype);

    char filesize[32];
    snprintf(filesize, sizeof(filesize), "%ld", file_content->size);
    req->header_add(req, "Content-Length", filesize);

    req->set_payload_file_content(req, file_content);

    http1response_t* res = client->send(client);
    if (!res)
        goto failed;
    if (res->status_code >= 300)
        goto failed;

    result = 1;

    failed:

    if (uri != NULL) free(uri);
    if (url != NULL) free(url);
    if (date != NULL) free(date);
    if (authorization != NULL) free(authorization);
    if (client != NULL) client->free(client);

    return result;
}

int __storages3_file_remove(void* storage, const char* path) {
    storages3_t* s = storage;
    int result = 0;

    const char* method = "DELETE";
    const char* mimetype = "";

    char* uri = __storages3_create_uri(s, path);
    if (uri == NULL) goto failed;

    char* url = __storages3_create_url(s, uri);
    if (url == NULL) goto failed;

    char* date = __storages3_create_date();
    if (date == NULL) goto failed;

    char* authorization = __storages3_create_authtoken(s, method, mimetype, date, uri);
    if (authorization == NULL) goto failed;

    const int timeout = 3;
    httpclient_t* client = httpclient_init(ROUTE_DELETE, url, timeout);
    if (client == NULL)
        goto failed;

    http1request_t* req = client->request;
    req->header_add(req, "Date", date);
    req->header_add(req, "Authorization", authorization);

    http1response_t* res = client->send(client);
    if (!res)
        goto failed;
    if (res->status_code != 200)
        goto failed;

    result = 1;

    failed:

    if (uri != NULL) free(uri);
    if (url != NULL) free(url);
    if (date != NULL) free(date);
    if (authorization != NULL) free(authorization);
    if (client != NULL) client->free(client);

    return result;
}

int __storages3_file_exist(void* storage, const char* path) {
    storages3_t* s = storage;
    int result = 0;

    const char* method = "HEAD";
    const char* mimetype = "";

    char* uri = __storages3_create_uri(s, path);
    if (uri == NULL) goto failed;

    char* url = __storages3_create_url(s, uri);
    if (url == NULL) goto failed;

    char* date = __storages3_create_date();
    if (date == NULL) goto failed;

    char* authorization = __storages3_create_authtoken(s, method, mimetype, date, uri);
    if (authorization == NULL) goto failed;

    const int timeout = 3;
    httpclient_t* client = httpclient_init(ROUTE_HEAD, url, timeout);
    if (client == NULL)
        goto failed;

    http1request_t* req = client->request;
    req->header_add(req, "Date", date);
    req->header_add(req, "Authorization", authorization);

    http1response_t* res = client->send(client);
    if (!res)
        goto failed;
    if (res->status_code != 200)
        goto failed;

    result = 1;

    failed:

    if (uri != NULL) free(uri);
    if (url != NULL) free(url);
    if (date != NULL) free(date);
    if (authorization != NULL) free(authorization);
    if (client != NULL) client->free(client);

    return result;
}

int __storages3_create_signature(storages3_t* storage, const char* data, char* result) {
    unsigned char out[EVP_MAX_MD_SIZE];
    unsigned int out_len = 0;

    HMAC(EVP_sha1(), storage->access_secret, strlen(storage->access_secret), (unsigned char*)data, strlen(data), out, &out_len);
    if (!out_len)
        return 0;

    return base64_encode(result, (const char*)out, out_len);
}

char* __storages3_create_uri(storages3_t* storage, const char* path) {
    size_t uri_length = strlen(storage->bucket) + strlen(path) + 2;
    char* uri = malloc(uri_length + 1);
    if (uri == NULL) return NULL;

    snprintf(uri, uri_length + 1, "/%s/%s", storage->bucket, path);
    storage_merge_slash(uri);

    return uri;
}

char* __storages3_create_url(storages3_t* storage, const char* uri) {
    char port[8];
    memset(port, 0, 8);
    if (storage->port[0] != 0)
        snprintf(port, sizeof(port), ":%s", storage->port);

    const size_t url_length = strlen(storage->protocol) + strlen(storage->host) + strlen(port) + strlen(uri) + 3;
    char* url = malloc(url_length + 1);
    if (url == NULL) return NULL;

    snprintf(url, url_length + 1, "%s://%s%s%s", storage->protocol, storage->host, port, uri);

    return url;
}

char* __storages3_create_date() {
    char* date = malloc(sizeof * date * 32);
    if (date == NULL) return NULL;

    time_t rawtime = time(0);
    strftime(date, 32, "%a, %d %b %Y %H:%M:%S ", localtime(&rawtime));

    const int tz = timezone_offset();
    const char* sign = tz >= 0 ? "+" : "-";
    const char* zero = tz < 10 ? "0" : "";
    char timezone[7];
    snprintf(timezone, sizeof(timezone), "%s%s%d00", sign, zero, tz);
    strcat(date, timezone);

    return date;
}

char *__storages3_create_signdata(const char *method, const char *mimetype, const char *date, const char *uri) {
    const size_t data_length = strlen(method) + strlen(mimetype) + strlen(date) + strlen(uri) + 4;
    char* data = malloc(data_length + 1);
    if (data == NULL) return NULL;

    snprintf(data, data_length + 1, "%s\n\n%s\n%s\n%s", method, mimetype, date, uri);

    return data;
}

char* __storages3_create_authtoken(storages3_t* storage, const char* method, const char* mimetype, const char* date, const char* uri) {
    char* data = __storages3_create_signdata(method, mimetype, date, uri);
    if (data == NULL) return NULL;

    char* result = NULL;
    char signature[32];
    const int signature_length = __storages3_create_signature(storage, data, &signature[0]);
    if (signature_length <= 0)
        goto failed;

    const size_t authtoken_length = strlen(storage->access_id) + signature_length + 5;
    char* authtoken = malloc(authtoken_length + 1);
    if (authtoken == NULL)
        goto failed;

    snprintf(authtoken, authtoken_length + 1, "AWS %s:%s", storage->access_id, signature);

    result = authtoken;

    failed:

    free(data);

    return result;
}
