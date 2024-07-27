#ifndef __DKIMCANONPARSER__
#define __DKIMCANONPARSER__

#include "bufferdata.h"

typedef enum dkimcanonparser_stage {
    DKIMCANONPARSER_SYMBOL = 0,
    DKIMCANONPARSER_SPACE
} dkimcanonparser_stage_e;

typedef struct dkimcanonparser {
    dkimcanonparser_stage_e stage;
    bufferdata_t buf;
    size_t buffer_size;
    size_t pos_start;
    size_t pos;
    const char* buffer;
} dkimcanonparser_t;

dkimcanonparser_t* dkimcanonparser_alloc();
void dkimcanonparser_init(dkimcanonparser_t* parser);
void dkimcanonparser_set_buffer(dkimcanonparser_t* parser, const char* buffer, const size_t buffer_size);
void dkimcanonparser_flush(dkimcanonparser_t* parser);
void dkimcanonparser_free(dkimcanonparser_t* parser);
int dkimcanonparser_run(dkimcanonparser_t* parser);
char* dkimcanonparser_get_content(dkimcanonparser_t* parser);

#endif