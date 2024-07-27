#include "websockets.h"
#include "mybroadcast.h"

mybroadcast_id_t* mybroadcast_id_create() {
    mybroadcast_id_t* st = malloc(sizeof * st);
    if (st == NULL) return NULL;

    st->base.free = mybroadcast_id_free;
    st->user_id = 1;
    st->project_id = 2;

    return st;
}

mybroadcast_id_t* mybroadcast_compare_id_create() {
    mybroadcast_id_t* st = malloc(sizeof * st);
    if (st == NULL) return NULL;

    st->base.free = mybroadcast_id_free;
    st->user_id = 1;
    st->project_id = 0;

    return st;
}

void mybroadcast_id_free(void* id) {
    mybroadcast_id_t* my_id = id;
    if (id == NULL) return;

    my_id->user_id = 0;
    my_id->project_id = 0;

    free(my_id);
}

void mybroadcast_send_data(response_t* response, const char* data, size_t size) {
    websocketsresponse_t* wsresponse = (websocketsresponse_t*)response;
    wsresponse->textn(wsresponse, data, size);
}

int mybroadcast_compare(void* sourceStruct, void* targetStruct) {
    mybroadcast_id_t* sd = sourceStruct;
    mybroadcast_id_t* td = targetStruct;

    return sd->user_id == td->user_id;
}