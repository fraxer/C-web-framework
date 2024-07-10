#include <unistd.h>
#include <stdlib.h>

#include "appconfig.h"
#include "multiplexing.h"
#include "multiplexingepoll.h"

mpxapi_t* mpx_create(mpxtype_e type) {
    mpxapi_t* result = NULL;
    mpxapi_t* api = NULL;

    switch (type) {
    case MULTIPLEX_TYPE_EPOLL:
        api = (mpxapi_t*)mpx_epoll_init();
        if (api == NULL) goto failed;
        break;
    default:
        api = NULL;
    }

    result = api;

    failed:

    if (result == NULL)
        mpx_free(api);

    return result;
}

void mpx_free(mpxapi_t* api) {
    if (api == NULL) return;

    if (api) api->free(api);
}

void mpxlistener_free(mpxlistener_t* listener) {
    if (listener == NULL) return;

    free(listener);
}
