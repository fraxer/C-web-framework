#ifndef __MAIL__
#define __MAIL__

#include <arpa/inet.h>

#include "mailheader.h"
#include "openssl.h"
#include "smtprequest.h"
#include "smtpresponse.h"

#define NS_MAXDNAME 1025

#define MAIL_IP_LIST_SIZE 5
#define MAIL_RECORDS_SIZE 5

typedef struct mail_payload {
    const char* from;
    const char* from_name;
    const char* to;
    const char* subject;
    const char* body;
} mail_payload_t;

typedef struct mail_string {
    char* value;
    size_t length;
} mail_string_t;

typedef struct mail {
    int reseted;
    mail_string_t from_with_name;
    mail_string_t from;
    mail_string_t to;
    mail_string_t subject;
    mail_string_t date;
    mail_string_t message_id;

    connection_t* connection;
    smtprequest_t* request;
    smtprequest_data_t* request_data;
    smtpresponse_t* response;

    char* buffer;
    size_t buffer_size;

    char* data;
    size_t data_size;

    SSL_CTX* ssl_ctx;
    mail_header_t* _header;
    mail_header_t* _last_header;

    int(*connected)(struct mail* instance);
    int(*connect)(struct mail* instance, const char* email);

    int(*read_banner)(struct mail* instance);
    int(*send_hello)(struct mail* instance);
    int(*start_tls)(struct mail* instance);
    int(*set_from)(struct mail* instance, const char* email, const char* sender_name);
    int(*set_to)(struct mail* instance, const char* email);
    int(*set_subject)(struct mail* instance, const char* subject);
    int(*set_body)(struct mail* instance, const char* body);

    int(*send_mail)(struct mail* instance);
    int(*send_reset)(struct mail* instance);
    int(*send_quit)(struct mail* instance);

    void(*free)(struct mail* instance);
} mail_t;

typedef struct mail_mx_record {
    int ok;
    int preference;
    char domain[NS_MAXDNAME];
    struct in_addr ip_list[MAIL_IP_LIST_SIZE];
} mail_mx_record_t;

mail_t* mail_create();
int mail_is_real(const char* email);
int send_mail(mail_payload_t* payload);
void send_mail_async(mail_payload_t* payload);

#endif