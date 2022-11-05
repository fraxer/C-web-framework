#ifndef __CONNECTION__
#define __CONNECTION__

typedef struct connection {
    int fd;
    int basefd;
    void* apidata;

    void(*close)(struct connection*);
    void(*read)(struct connection*);
    void(*write)(struct connection*);
} connection_t;

connection_t* connection_create(int, int, int(*)(connection_t*));

connection_t* connection_alloc(int fd, int basefd);

#endif
