#ifndef __BUFFERDATA__
#define __BUFFERDATA__

#include <unistd.h>

#define BUFFERDATA_SIZE 4096

typedef enum bufferdata_type {
    BUFFERDATA_STATIC = 0,
    BUFFERDATA_DYNAMIC,
} bufferdata_type_e;

typedef struct bufferdata {
    char static_buffer[BUFFERDATA_SIZE];
    char* dynamic_buffer;
    size_t offset_sbuffer;
    size_t offset_dbuffer;
    size_t dbuffer_size;
    bufferdata_type_e type;
} bufferdata_t;

void bufferdata_init(bufferdata_t*);
void bufferdata_reset(bufferdata_t*);

size_t bufferdata_writed(bufferdata_t*);
int bufferdata_push(bufferdata_t*, char);
int bufferdata_complete(bufferdata_t*);
int bufferdata_move(bufferdata_t*);

char* bufferdata_get(bufferdata_t*);
char* bufferdata_copy(bufferdata_t*);

#endif