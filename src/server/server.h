#ifndef __SERVER__
#define __SERVER__

#include <arpa/inet.h>
#include "../redirect/redirect.h"
#include "../route/route.h"
#include "../route/routeloader.h"
#include "../database/database.h"
#include "../domain/domain.h"
#include "../openssl/openssl.h"

typedef struct index {
    char* value;
    int length;
} index_t;

typedef struct server_info {
    int read_buffer;
    int client_max_body_size;
    enum { ENV_DEV, ENV_PROD } environment;
    char* tmp_dir;
} server_info_t;

typedef struct server_http {
    route_t* route;
    redirect_t* redirect;
} server_http_t;

typedef struct server_websockets {
    route_t* route;
} server_websockets_t;

typedef struct server {
    unsigned short int port;
    size_t root_length;
    in_addr_t ip;
    server_http_t http;
    server_websockets_t websockets;

    char* root;
    domain_t* domain;
    index_t* index;
    database_t* database;
    openssl_t* openssl;
    server_info_t* info;
    struct server* next;
} server_t;

typedef struct server_chain {
    int is_deprecated;
    int is_hard_reload;
    int thread_count;
    int domain_hash_bucket_size;

    pthread_mutex_t mutex;

    server_t* server;
    server_info_t* info;
    routeloader_lib_t* routeloader;
    struct server_chain* prev;
    struct server_chain* next;
    void(*destroy)(struct server_chain*);
} server_chain_t;

server_t* server_create();

index_t* server_create_index(const char*);

void server_free(server_t*);

server_chain_t* server_chain_create(server_t* server, routeloader_lib_t*, server_info_t*, int);

server_chain_t* server_chain_last();

int server_chain_append(server_t*, routeloader_lib_t*, server_info_t*, int);

void server_chain_destroy(server_chain_t*);

server_info_t* server_info_alloc();

void server_info_free(server_info_t*);

#endif
