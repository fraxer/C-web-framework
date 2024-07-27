#ifndef __HTTPCLIENT__
#define __HTTPCLIENT__

#include <stddef.h>
#include <openssl/ssl.h>

#include "route.h"
#include "connection.h"
#include "http1common.h"
#include "http1request.h"
#include "http1response.h"
#include "httpclientparser.h"

typedef enum client_redirect {
    CLIENTREDIRECT_NONE = 0,
    CLIENTREDIRECT_EXIST,
    CLIENTREDIRECT_ERROR,
    CLIENTREDIRECT_MANY_REDIRECTS
} client_redirect_e;

typedef struct httpclient {
    route_methods_e method;
    unsigned int use_ssl : 1;
    short int redirect_count;
    short int port;
    int timeout;
    char* host;
    SSL_CTX* ssl_ctx;
    connection_t* connection;
    http1request_t* request;
    http1response_t* response;
    httpclientparser_t* parser;
    http1response_t*(*send)();
    const char*(*error)(struct httpclient*);
    void(*set_method)(struct httpclient*, route_methods_e);
    int(*set_url)(struct httpclient*, const char*);
    void(*set_timeout)(struct httpclient*, int);
    void(*free)(struct httpclient*);
} httpclient_t;

httpclient_t* httpclient_init(route_methods_e method, const char* url, int timeout);

#endif