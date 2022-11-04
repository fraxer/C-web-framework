#ifndef __SERVER__
#define __SERVER__

#include <arpa/inet.h>
#include "../route/route.h"
#include "../domain/domain.h"

typedef struct index {
    char* value;
    int length;
} index_t;

typedef struct redirect {
    char* template;
    char* target;
    struct redirect* next;
} redirect_t;

typedef struct server {
    unsigned short int port;
    domain_t* domain;
    in_addr_t ip;
    char* root;
    index_t* index;
    redirect_t* redirect;
    route_t* route;
    // database_t* database;
    struct server* next;
} server_t;

void* server_iinit();

server_t* server_create();

server_t* server_alloc();

index_t* server_create_index(const char*);

void server_free(server_t*);

server_t* server_get_first();

#endif
