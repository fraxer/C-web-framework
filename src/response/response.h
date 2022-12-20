#ifndef __RESPONSE__
#define __RESPONSE__

typedef struct response {
    void(*free)(void*);
} response_t;

#endif
