#ifndef __DKIMHEADERPARSER__
#define __DKIMHEADERPARSER__

#include "bufferdata.h"

typedef enum dkimheaderparser_stage {
    DKIMHEADERPARSER_SYMBOL = 0,
    DKIMHEADERPARSER_SPACE
} dkimheaderparser_stage_e;

typedef struct dkimheaderparser {
    dkimheaderparser_stage_e stage;
    bufferdata_t buf;
    size_t buffer_size;
    size_t pos_start;
    size_t pos;
    const char* buffer;
} dkimheaderparser_t;

dkimheaderparser_t* dkimheaderparser_alloc();
void dkimheaderparser_init(dkimheaderparser_t* parser);
void dkimheaderparser_set_buffer(dkimheaderparser_t* parser, const char* buffer, const size_t buffer_size);
void dkimheaderparser_flush(dkimheaderparser_t* parser);
void dkimheaderparser_free(dkimheaderparser_t* parser);
int dkimheaderparser_run(dkimheaderparser_t* parser);
char* dkimheaderparser_get_content(dkimheaderparser_t* parser);
size_t dkimheaderparser_get_content_length(dkimheaderparser_t* parser);

#endif