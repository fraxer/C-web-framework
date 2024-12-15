#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "log.h"
#include "appconfig.h"
#include "base64.h"
#include "helpers.h"
#include "mimetype.h"
#include "storages3.h"
#include "httpclient.h"

static void __storages3_free(void* storage);
static file_t __storages3_file_get(void* storage, const char* path);
static int __storages3_file_put(void* storage, const file_t* file, const char* path);
static int __storages3_file_content_put(void* storage, const file_content_t* file_content, const char* path);
static int __storages3_file_remove(void* storage, const char* path);
static int __storages3_file_exist(void* storage, const char* path);
static array_t* __storages3_file_list(void* storage, const char* path);
static int __storages3_create_signature(storages3_t* storage, const char* data, char* result);
static char* __storages3_create_uri(storages3_t* storage, const char* path_format, ...);
static char* __storages3_create_url(storages3_t* storage, const char* uri);
static char* __storages3_create_date();
static char* __storages3_create_signdata(const char* method, const char* mimetype, const char* date, const char* uri);
static char* __storages3_create_authtoken(storages3_t* storage, const char* method, const char* mimetype, const char* date, const char* uri);
static char* __storages3_create_authtoken2(storages3_t* storage, httpclient_t* client, const char* method, const char* date, const char* payload_hash);
static int __host_str(storages3_t* storage, char* string);
static void __get_amz_date(char* date_str, size_t date_size);
static void __get_short_date(char* short_date, size_t short_date_size);
static int __file_sha256(const int fd, unsigned char* hash);

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
    storage->base.file_list = __storages3_file_list;

    return storage;
}

void __storages3_free(void* storage) {
    free(storage);
}

file_t __storages3_file_get(void* storage, const char* path) {
    storages3_t* s = storage;
    file_t result = file_alloc();

    const char* method = "GET";
    char* uri = NULL;
    char* url = NULL;
    // char* date = NULL;
    char* authorization = NULL;
    httpclient_t* client = NULL;

    uri = __storages3_create_uri(s, path);
    if (uri == NULL) goto failed;

    url = __storages3_create_url(s, uri);
    if (url == NULL) goto failed;

    // date = __storages3_create_date();
    // if (date == NULL) goto failed;

    // authorization = __storages3_create_authtoken(s, method, mimetype, date, uri);
    // if (authorization == NULL) goto failed;

    const int timeout = 3;
    client = httpclient_init(ROUTE_GET, url, timeout);
    if (client == NULL)
        goto failed;

    http1request_t* req = client->request;

    const char* payload_hash = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    char amz_date[64];
    __get_amz_date(amz_date, sizeof(amz_date));
    authorization = __storages3_create_authtoken2(s, client, method, amz_date, payload_hash);
    if (authorization == NULL) goto failed;

    req->header_add(req, "Authorization", authorization);
    req->header_add(req, "x-amz-content-sha256", payload_hash);
    req->header_add(req, "x-amz-date", amz_date);

    // req->header_add(req, "Date", date);
    // req->header_add(req, "Authorization", authorization);

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
    // if (date != NULL) free(date);
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
    const char* mimetype = mimetype_find_type(appconfig()->mimetype, ext);
    if (mimetype == NULL)
        mimetype = "text/plain";

    char* uri = NULL;
    char* url = NULL;
    // char* date = NULL;
    char* authorization = NULL;
    httpclient_t* client =  NULL;

    uri = __storages3_create_uri(s, path);
    if (uri == NULL) goto failed;

    url = __storages3_create_url(s, uri);
    if (url == NULL) goto failed;

    // date = __storages3_create_date();
    // if (date == NULL) goto failed;

    // authorization = __storages3_create_authtoken(s, method, mimetype, date, uri);
    // if (authorization == NULL) goto failed;

    const int timeout = 3;
    client = httpclient_init(ROUTE_PUT, url, timeout);
    if (client == NULL)
        goto failed;

    http1request_t* req = client->request;

    unsigned char file_content_hash[SHA256_DIGEST_LENGTH];
    char payload_hash[SHA256_DIGEST_LENGTH * 2 + 1];

    if (!__file_sha256(file_content->fd, file_content_hash))
        goto failed;

    bytes_to_hex(file_content_hash, SHA256_DIGEST_LENGTH, payload_hash);

    char amz_date[64];
    __get_amz_date(amz_date, sizeof(amz_date));
    authorization = __storages3_create_authtoken2(s, client, method, amz_date, payload_hash);
    if (authorization == NULL) goto failed;

    req->header_add(req, "Authorization", authorization);
    req->header_add(req, "x-amz-content-sha256", payload_hash);
    req->header_add(req, "x-amz-date", amz_date);

    char content_disposition[512];
    snprintf(content_disposition, sizeof(content_disposition), "attachment; filename=%s", file_content->filename);
    req->header_add(req, "Content-Disposition", content_disposition);
    // req->header_add(req, "Date", date);
    // req->header_add(req, "Authorization", authorization);
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
    // if (date != NULL) free(date);
    if (authorization != NULL) free(authorization);
    if (client != NULL) client->free(client);

    return result;
}

int __storages3_file_remove(void* storage, const char* path) {
    storages3_t* s = storage;
    int result = 0;

    const char* method = "DELETE";
    const char* mimetype = "";
    char* uri = NULL;
    char* url = NULL;
    char* date = NULL;
    char* authorization = NULL;
    httpclient_t* client =  NULL;

    uri = __storages3_create_uri(s, path);
    if (uri == NULL) goto failed;

    url = __storages3_create_url(s, uri);
    if (url == NULL) goto failed;

    date = __storages3_create_date();
    if (date == NULL) goto failed;

    authorization = __storages3_create_authtoken(s, method, mimetype, date, uri);
    if (authorization == NULL) goto failed;

    const int timeout = 3;
    client = httpclient_init(ROUTE_DELETE, url, timeout);
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
    char* uri = NULL;
    char* url = NULL;
    char* date = NULL;
    char* authorization = NULL;
    httpclient_t* client = NULL;

    uri = __storages3_create_uri(s, path);
    if (uri == NULL) goto failed;

    url = __storages3_create_url(s, uri);
    if (url == NULL) goto failed;

    date = __storages3_create_date();
    if (date == NULL) goto failed;

    authorization = __storages3_create_authtoken(s, method, mimetype, date, uri);
    if (authorization == NULL) goto failed;

    const int timeout = 3;
    client = httpclient_init(ROUTE_HEAD, url, timeout);
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

array_t* __storages3_file_list(void* storage, const char* path) {
    storages3_t* s = storage;

    const char* method = "GET";
    char* uri = NULL;
    char* url = NULL;
    char* date = NULL;
    char* authorization = NULL;
    httpclient_t* client = NULL;
    array_t* list = NULL;

    uri = __storages3_create_uri(s, "?delimiter=/&max-keys=1000&prefix=%s", path);
    if (uri == NULL) goto failed;

    url = __storages3_create_url(s, uri);
    if (url == NULL) goto failed;

    date = __storages3_create_date();
    if (date == NULL) goto failed;

    const int timeout = 3;
    client = httpclient_init(ROUTE_GET, url, timeout);
    if (client == NULL) goto failed;

    http1request_t* req = client->request;

    const char* payload_hash = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    char amz_date[64];
    __get_amz_date(amz_date, sizeof(amz_date));
    authorization = __storages3_create_authtoken2(s, client, method, amz_date, payload_hash);
    if (authorization == NULL) goto failed;

    req->header_add(req, "Authorization", authorization);
    req->header_add(req, "x-amz-content-sha256", payload_hash);
    req->header_add(req, "x-amz-date", amz_date);

    http1response_t* res = client->send(client);
    if (!res)
        goto failed;
    if (res->status_code != 200)
        goto failed;

    list = array_create();
    if (list == NULL) goto failed;

    char* payload = res->payload(res);

    if (!parse_s3_response(payload)) {
        goto failed;
    }

    // parse file list

    failed:

    if (uri != NULL) free(uri);
    if (url != NULL) free(url);
    if (date != NULL) free(date);
    if (authorization != NULL) free(authorization);
    if (client != NULL) client->free(client);

    return list;
}

int __storages3_create_signature(storages3_t* storage, const char* data, char* result) {
    unsigned char out[EVP_MAX_MD_SIZE];
    unsigned int out_len = 0;

    HMAC(EVP_sha1(), storage->access_secret, strlen(storage->access_secret), (unsigned char*)data, strlen(data), out, &out_len);
    if (!out_len)
        return 0;

    return base64_encode(result, (const char*)out, out_len);
}

char* __storages3_create_uri(storages3_t* storage, const char* path_format, ...) {
    char path[PATH_MAX];
    va_list args;
    va_start(args, path_format);
    vsnprintf(path, sizeof(path), path_format, args);
    va_end(args);

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

/**
 * Creates a date string in the format "Day, DD Mon YYYY HH:MM:SS ±HHMM".
 * The function allocates memory for the date string, which must be freed
 * by the caller. The date string includes the current local time with
 * an appended timezone offset.
 *
 * @return A pointer to the allocated date string, or NULL if memory allocation fails.
 */
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



int __host_str(storages3_t* storage, char* string) {
    if (strlen(storage->host) + strlen(storage->port) > NAME_MAX - 2)
        return 0;

    strcpy(string, storage->host);

    if (storage->port[0] != 0) {
        strcat(string, ":");
        strcat(string, storage->port);
    }

    return 1;
}

void __get_amz_date(char* date_str, size_t date_size) {
    time_t now = time(NULL);
    strftime(date_str, date_size, "%Y%m%dT%H%M%SZ", gmtime(&now));
}

void __get_short_date(char* short_date, size_t short_date_size) {
    time_t now = time(NULL);
    strftime(short_date, short_date_size, "%Y%m%d", gmtime(&now));
}

void calculate_signing_key(const char* secret_key, const char* short_date, const char *region, const char *service, unsigned char* signing_key) {
    unsigned char k_date[SHA256_DIGEST_LENGTH];
    unsigned char k_region[SHA256_DIGEST_LENGTH];
    unsigned char k_service[SHA256_DIGEST_LENGTH];
    unsigned char k_signing[SHA256_DIGEST_LENGTH];

    char _signing_key[264] = "AWS4";
    strncat(_signing_key, secret_key, sizeof(_signing_key) - strlen(_signing_key) - 1);

    HMAC(EVP_sha256(), 
         (unsigned char *)(_signing_key), 
         strlen(_signing_key), 
         (unsigned char *)short_date, 
         strlen(short_date), 
         (unsigned char *)k_date, NULL);
    
    // k_region
    HMAC(EVP_sha256(), 
         (unsigned char *)k_date, 
         SHA256_DIGEST_LENGTH, 
         (unsigned char *)region, 
         strlen(region), 
         (unsigned char *)k_region, NULL);
    
    // k_service
    HMAC(EVP_sha256(), 
         (unsigned char *)k_region, 
         SHA256_DIGEST_LENGTH, 
         (unsigned char *)service, 
         strlen(service), 
         (unsigned char *)k_service, NULL);
    
    // k_signing
    HMAC(EVP_sha256(), 
         (unsigned char *)k_service, 
         SHA256_DIGEST_LENGTH, 
         (unsigned char *)"aws4_request", 
         strlen("aws4_request"), 
         (unsigned char *)k_signing, NULL);
    
    memcpy(signing_key, k_signing, SHA256_DIGEST_LENGTH);
}

void calculate_signature(const unsigned char* signing_key,const char* string_to_sign,char* signature) {
    unsigned char signature_bytes[SHA256_DIGEST_LENGTH];
    
    HMAC(EVP_sha256(),
         signing_key,
         SHA256_DIGEST_LENGTH,
         (unsigned char*)string_to_sign,
         strlen(string_to_sign),
         signature_bytes, NULL);
    
    bytes_to_hex(signature_bytes, SHA256_DIGEST_LENGTH, signature);
}

char* __storages3_create_authtoken2(storages3_t* storage, httpclient_t* client, const char* method, const char* date, const char* payload_hash) {
    char short_date[64];
    __get_short_date(short_date, sizeof(short_date));
    const char *region = "us-east-1";
    const char *service = "s3";

    char host[NAME_MAX] = {0};
    if (!__host_str(storage, host)) return NULL;

    char canonical_headers[350];
    snprintf(canonical_headers, sizeof(canonical_headers), 
        "host:%s\nx-amz-content-sha256:%s\nx-amz-date:%s\n",
        host,
        payload_hash,
        date
    );

    const char *signed_headers = "host;x-amz-content-sha256;x-amz-date";
    httpclientparser_t* parser = client->parser;

    char* query_str = http1_query_str(parser->query);

    char canonical_request[1024];
    snprintf(canonical_request, 1024, 
        "%s\n%s\n%s\n%s\n%s\n%s",
        method,
        parser->path,
        query_str != NULL ? query_str : "",
        canonical_headers,
        signed_headers,
        payload_hash
    );

    if (query_str != NULL)
        free(query_str);

    unsigned char canonical_request_hash[SHA256_DIGEST_LENGTH];
    char canonical_request_hash_hex[SHA256_DIGEST_LENGTH * 2 + 1];
    SHA256((unsigned char *)canonical_request, strlen(canonical_request), canonical_request_hash);
    bytes_to_hex(canonical_request_hash, SHA256_DIGEST_LENGTH, canonical_request_hash_hex);

    char string_to_sign[1024];
    snprintf(string_to_sign, 1024,
        "AWS4-HMAC-SHA256\n%s\n%s/%s/%s/aws4_request\n%s",
        date,
        short_date,
        region,
        service,
        canonical_request_hash_hex
    );

    unsigned char signing_key[SHA256_DIGEST_LENGTH];
    calculate_signing_key(
        storage->access_secret,
        short_date,
        region,
        service,
        signing_key
    );

    char signature[SHA256_DIGEST_LENGTH * 2 + 1];
    calculate_signature(
        signing_key,
        string_to_sign,
        signature
    );

    char* authorization_header = malloc(512);
    if (authorization_header == NULL) return NULL;

    snprintf(authorization_header, 512, 
        "AWS4-HMAC-SHA256 Credential=%s/%s/%s/%s/aws4_request,SignedHeaders=%s,Signature=%s", 
        storage->access_id, 
        short_date,
        region,
        service,
        signed_headers, 
        signature
    );

    return authorization_header;
}

int __file_sha256(const int fd, unsigned char* hash) {
    SHA256_CTX sha256;
    unsigned char buffer[4096];
    ssize_t bytes_read = 0;
    int result = 0;

    if (fd == -1) {
        log_error("__file_sha256: Error opening file fd %d: %s\n", fd, strerror(errno));
        goto failed;
    }

    if (SHA256_Init(&sha256) != 1) {
        log_error("__file_sha256: SHA256 initialization failed\n");
        goto failed;
    }

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        if (SHA256_Update(&sha256, buffer, bytes_read) != 1) {
            log_error("__file_sha256: SHA256 update failed\n");
            goto failed;
        }
    }

    if (bytes_read == -1) {
        log_error("__file_sha256: Error reading file fd %d: %s\n", fd, strerror(errno));
        goto failed;
    }

    if (SHA256_Final(hash, &sha256) != 1) {
        log_error("__file_sha256: SHA256 finalization failed\n");
        goto failed;
    }

    result = 1;

    failed:

    lseek(fd, 0, SEEK_SET);

    return result;
}

int parse_s3_response(const char* response) {
    // xmlParserCtxtPtr parser = xmlNewParserCtxt();
    // if (parser == NULL) {
    //     log_error("Failed to create xml parser\n");
    //     return 0;
    // }

    // Parse the response string
    xmlDocPtr doc = xmlParseMemory(response, strlen(response));
    if (doc == NULL) {
        // Handle error
    }

    // Get the root element
    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (root == NULL) {
        // Handle error
    }

    // Iterate over the elements
    xmlNodePtr node = root->children;
    while (node != NULL) {
        // Get the element name
        const char *name = node->name;

        printf("Node key: %s\n", name);

        // Check if the element is a "Contents" element
        if (strcmp(name, "Contents") == 0) {
            // Get the "Key" element
            xmlNodePtr key_node = node->children;
            while (key_node != NULL) {
                if (strcmp(key_node->name, "Key") == 0) {
                    // Get the key value
                    const char *key = xmlNodeGetContent(key_node);
                    printf("Key: %s\n", key);
                    xmlFree(key);
                }
                key_node = key_node->next;
            }
        }

        node = node->next;
    }

    // Clean up
    xmlFreeDoc(doc);
    // xmlFreeParserCtxt(parser);
}
