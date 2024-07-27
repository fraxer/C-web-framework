#ifndef __DKIM__
#define __DKIM__

#include "mailheader.h"

typedef struct {
    const char* private_key;
    const char* domain;
    const char* selector;
    time_t timestamp;

    mail_header_t* header;
    mail_header_t* last_header;
} dkim_t;

dkim_t* dkim_create();
int dkim_header_add(dkim_t* dkim, const char* key, const size_t key_length, const char* value, const size_t value_length);
void dkim_set_private_key(dkim_t* dkim, const char* private_key);
void dkim_set_domain(dkim_t* dkim, const char* domain);
void dkim_set_selector(dkim_t* dkim, const char* selector);
void dkim_set_timestamp(dkim_t* dkim, const time_t timestamp);
char* dkim_create_sign(dkim_t* dkim, const char* body);
void dkim_free(dkim_t* dkim);

#endif