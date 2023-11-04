#ifndef __MYBROADCAST__
#define __MYBROADCAST__

#include "broadcast.h"

typedef struct mybroadcast_id {
    broadcast_id_t base;
    int user_id;
    int project_id;
} mybroadcast_id_t;

mybroadcast_id_t* mybroadcast_id_create();
mybroadcast_id_t* mybroadcast_compare_id_create();
void mybroadcast_id_free(void*);
void mybroadcast_send_data(response_t*, const char*, size_t);
int mybroadcast_compare(void*, void*);

#endif
