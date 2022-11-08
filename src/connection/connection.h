#ifndef __CONNECTION__
#define __CONNECTION__

#include <pthread.h>

typedef struct connection {
    int fd;
    int basefd;
    int ssl_enabled;
    int keepalive_enabled;
    void* ssl;
    void* apidata;
    void* protocol;

    pthread_mutex_t mutex;

    int(*close)(struct connection*);
    void(*read)(struct connection*, char*, size_t);
    void(*write)(struct connection*);
    int(*after_read_request)(struct connection*);
    int(*after_write_request)(struct connection*);
    int(*switch_to_http1)(struct connection*);
    int(*switch_to_websocket)(struct connection*);
} connection_t;

connection_t* connection_create(int, int, int(*)(connection_t*));

connection_t* connection_alloc(int fd, int basefd);

void connection_free(connection_t*);

int connection_trylock(connection_t*);

int connection_unlock(connection_t*);

#endif
