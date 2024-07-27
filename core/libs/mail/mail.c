#define _GNU_SOURCE
#include <string.h>
#include <sys/time.h>
#include <idn2.h>
#include <resolv.h>
#include <arpa/nameser.h>
#include <netdb.h>

#include "log.h"
#include "appconfig.h"
#include "helpers.h"
#include "base64.h"
#include "protocolmanager.h"
#include "dkim.h"
#include "mail.h"

void __mail_string_reset(mail_string_t* string);
void __mail_string_free(mail_string_t* string);
int __mail_connected(mail_t* instance);
int __mail_connect(mail_t* instance, const char* email);
connection_t* __mail_connection_create();
int __mail_connection_close(connection_t* connection);
void __mail_connection_free(connection_t* connection);
int __mail_read_banner(mail_t* instance);
int __mail_send_hello(mail_t* instance);
int __mail_start_tls(mail_t* instance);
int __mail_send_quit(mail_t* instance);
int __mail_set_from(mail_t* instance, const char* email, const char* sender_name);
int __mail_set_to(mail_t* instance, const char* email);
int __mail_set_subject(mail_t* instance, const char* subject);
int __mail_set_date(mail_t* instance, time_t* rawtime);
int __mail_set_message_id(mail_t* instance, time_t* rawtime);
int __mail_set_content(mail_t* instance, const char* body);
int __mail_send_mail(mail_t* instance);
int __mail_send_from(mail_t* instance);
int __mail_send_to(mail_t* instance);
int __mail_send_data(mail_t* instance);
int __mail_send_content(mail_t* instance);
int __mail_send_reset(mail_t* instance);
void __mail_free(mail_t* instance);
const char* __mail_domain_from_email(const char* email);
int __mail_set_conn_timeout(const int fd);
int __mail_get_mx_servers(const char* host, mail_mx_record_t* mx_records);
int __mail_parse_mx_record(unsigned char* buffer, size_t r, ns_sect s, int idx, ns_msg* message, mail_mx_record_t* mx_record);
int __mail_can_interact(mail_t* instance);
int __mail_send_command(mail_t* instance, const char* format, ...);
int __mail_init_tls(mail_t* instance);
int __mail_alloc_ssl(connection_t* connection);
int __mail_handshake(connection_t* connection);
int __mail_set_dkim_headers(dkim_t* dkim, mail_t* instance);
int __mail_build_content(mail_t* instance);
int __mail_header_add(mail_t* instance, const char* key, const char* value);
size_t __mail_calc_content_length(mail_t* instance);
int __mail_data_append(char* data, size_t* pos, const char* string, const size_t length);
int __mail_after_read(connection_t* connection);
int __mail_after_write(connection_t* connection);


mail_t* mail_create() {
    mail_t* instance = malloc(sizeof * instance);
    if (instance == NULL) return NULL;

    instance->reseted = 0;

    __mail_string_reset(&instance->from_with_name);
    __mail_string_reset(&instance->from);
    __mail_string_reset(&instance->to);
    __mail_string_reset(&instance->subject);
    __mail_string_reset(&instance->date);
    __mail_string_reset(&instance->message_id);

    instance->ssl_ctx = NULL;
    instance->connection = NULL;
    instance->buffer_size = env()->main.buffer_size;
    instance->buffer = malloc(sizeof(char) * instance->buffer_size);
    if (instance->buffer == NULL) {
        free(instance);
        return NULL;
    }

    instance->data_size = 0;
    instance->data = NULL;
    instance->_header = NULL;
    instance->_last_header = NULL;
    instance->connected = __mail_connected;
    instance->connect = __mail_connect;
    instance->read_banner = __mail_read_banner;
    instance->send_hello = __mail_send_hello;
    instance->start_tls = __mail_start_tls;
    instance->set_from = __mail_set_from;
    instance->set_to = __mail_set_to;
    instance->set_subject = __mail_set_subject;
    instance->set_body = __mail_set_content;
    instance->send_mail = __mail_send_mail;
    instance->send_reset = __mail_send_reset;
    instance->send_quit = __mail_send_quit;
    instance->free = __mail_free;

    return instance;
}

int mail_is_real(const char* email) {
    if (email == NULL) return 0;

    const char* domain = __mail_domain_from_email(email);
    if (domain == NULL) {
        log_error("Domain not detected by email: %s\n", email);
        return 0;
    }

    char* punycode_domain = NULL;
    int r = idn2_to_ascii_8z(domain, &punycode_domain, IDN2_NONTRANSITIONAL);
    if (r != IDNA_SUCCESS) {
        log_error("Mail idn2_to_ascii_8z failed (%d): %s\n", r, idn2_strerror(r));
        return 0;
    }

    mail_mx_record_t mx_records[MAIL_RECORDS_SIZE];
    memset(mx_records, 0, sizeof(mail_mx_record_t) * MAIL_RECORDS_SIZE);
    r = __mail_get_mx_servers(punycode_domain, mx_records);

    free(punycode_domain);

    return r > 0;
}

int send_mail(mail_payload_t* payload) {
    if (payload == NULL) return 0;
    if (!mail_is_real(payload->to)) return 0;

    mail_t* mail = mail_create();
    if (!mail->connected(mail)) {
        if (!mail->connect(mail, payload->to))
            return 0;

        mail->read_banner(mail);
        mail->send_hello(mail);
        mail->start_tls(mail);
    }

    mail->set_from(mail, payload->from, payload->from_name);
    mail->set_to(mail, payload->to);
    mail->set_subject(mail, payload->subject);
    mail->set_body(mail, payload->body);

    mail->send_mail(mail);
    mail->send_reset(mail);
    mail->send_quit(mail);
    mail->free(mail);

    return 1;
}

void send_mail_async(mail_payload_t* payload) {
    (void)payload;
}

void __mail_string_reset(mail_string_t* item) {
    if (item == NULL) return;

    item->value = NULL;
    item->length = 0;
}

void __mail_string_free(mail_string_t* string) {
    if (string->value != NULL)
        free(string->value);

    __mail_string_reset(string);
}

int __mail_connected(mail_t* instance) {
    if (instance == NULL) return 0;
    if (instance->connection == NULL) return 0;

    return instance->connection->fd > 0;
}

int __mail_connect(mail_t* instance, const char* email) {
    if (instance == NULL) return 0;
    if (email == NULL) return 0;

    const char* domain = __mail_domain_from_email(email);
    if (domain == NULL) {
        log_error("Domain not detected by email: %s\n", email);
        return 0;
    }

    instance->connection = __mail_connection_create();
    if (instance->connection == NULL) {
        log_error("Error mail connection create\n");
        return 0;
    }

    instance->request_data = smtprequest_data_create(instance->connection);
    if (instance->request_data == NULL)
        return 0;

    instance->request = smtprequest_create(instance->connection);
    if (instance->request == NULL)
        return 0;

    instance->connection->request = (request_t*)instance->request;

    instance->response = smtpresponse_create(instance->connection);
    if (instance->response == NULL)
        return 0;

    instance->connection->response = (response_t*)instance->response;

    char* punycode_domain = NULL;
    int r = idn2_to_ascii_8z(domain, &punycode_domain, IDN2_NONTRANSITIONAL);
    if (r != IDNA_SUCCESS) {
        log_error("Mail idn2_to_ascii_8z failed (%d): %s\n", r, idn2_strerror(r));
        return 0;
    }

    log_info("Mail domain: %s", punycode_domain);

    mail_mx_record_t mx_records[MAIL_RECORDS_SIZE];
    memset(mx_records, 0, sizeof(mail_mx_record_t) * MAIL_RECORDS_SIZE);
    r = __mail_get_mx_servers(punycode_domain, mx_records);

    free(punycode_domain);

    if (!r) {
        log_error("[__mail_connect] Not found servers\n");
        return 0;
    }

    for (int i = 0; i < MAIL_RECORDS_SIZE; i++) {
        mail_mx_record_t* record = &mx_records[i];
        if (!record->ok) continue;

        for (int j = 0; j < MAIL_IP_LIST_SIZE; j++) {
            log_info("MX server ip: %d\n", record->ip_list[j].s_addr);

            if (record->ip_list[j].s_addr == 0) continue;

            struct sockaddr_in sockaddr;
            memset(&sockaddr, 0, sizeof(sockaddr));

            sockaddr.sin_family = AF_INET;
            sockaddr.sin_port = htons(instance->connection->port);
            sockaddr.sin_addr.s_addr = record->ip_list[j].s_addr;

            if (connect(instance->connection->fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == -1) {
                log_error("[createConnection] Error in connect\n");
                continue;
            }

            return 1;
        }
    }

    return 0;
}

connection_t* __mail_connection_create() {
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        log_error("Error mail socket create\n");
        return NULL;
    }

    if (!__mail_set_conn_timeout(fd)) {
        log_error("Error mail set socket timeout\n");
        return NULL;
    }

    connection_t* connection = connection_alloc(fd, NULL, 0, 0);
    if (connection == NULL) return NULL;

    connection->port = 25;
    connection->after_read_request = __mail_after_read;
    connection->after_write_request = __mail_after_write;
    connection->close = __mail_connection_close;

    protmgr_set_smtp_client_command(connection);

    return connection;
}

int __mail_connection_close(connection_t* connection) {
    if (connection == NULL) return 0;

    if (connection->fd > 0) {
        close(connection->fd);
        connection->fd = 0;
    }

    return 1;
}

void __mail_connection_free(connection_t* connection) {
    if (connection == NULL) return;

    connection->close(connection);

    if (connection->ssl != NULL) {
        SSL_free(connection->ssl);
        connection->ssl = NULL;
    }

    cqueue_free(connection->queue);

    free(connection);
}

int __mail_read_banner(mail_t* instance) {
    if (!__mail_can_interact(instance)) return 0;

    instance->connection->read(instance->connection, instance->buffer, instance->buffer_size);

    smtpresponse_t* response = (smtpresponse_t*)instance->connection->response;
    if (response->status != 220 && response->status != 250) {
        instance->reseted = 1;
        log_error("%s\n", response->message);
        return 0;
    }

    return 1;
}

int __mail_send_hello(mail_t* instance) {
    if (!__mail_can_interact(instance)) return 0;
    if (!__mail_send_command(instance, "EHLO %s\r\n", env()->mail.host)) return 0;

    return 1;
}

int __mail_start_tls(mail_t* instance) {
    if (!__mail_can_interact(instance)) return 0;
    if (!__mail_send_command(instance, "STARTTLS\r\n")) return 0;

    smtpresponse_t* response = (smtpresponse_t*)instance->connection->response;
    if (response->status != 220) return 0;

    if (!__mail_init_tls(instance)) return 0;
    if (!__mail_send_hello(instance)) return 0;

    return 1;
}

int __mail_send_quit(mail_t* instance) {
    if (instance == NULL) return 0;
    if (!__mail_connected(instance)) {
        log_error("[__mail_send_quit] Not connected\n");
        return 0;
    }

    if (!__mail_send_command(instance, "QUIT\r\n")) return 0;

    return 1;
}

int __mail_set_from(mail_t* instance, const char* email, const char* sender_name) {
    if (instance == NULL) return 0;
    if (email == NULL) return 0;
    if (sender_name == NULL) return 0;

    size_t sender_name_length = strlen(sender_name);
    char encoded_sender_name[base64_encode_len(sender_name_length)];
    size_t encoded_sender_name_length = base64_encode(encoded_sender_name, sender_name, sender_name_length);

    const size_t email_length = strlen(email);
    const char* template = "=?UTF-8?B?%s?= <%s>";
    instance->from_with_name.length = strlen(template) - 4 + encoded_sender_name_length + email_length;
    instance->from_with_name.value = malloc(instance->from_with_name.length + 1);
    if (instance->from_with_name.value == NULL)
        return 0;

    instance->from_with_name.length = snprintf(instance->from_with_name.value, instance->from_with_name.length + 1, template, encoded_sender_name, email);
    if (instance->from_with_name.length <= 0) return 0;

    template = "<%s>";
    instance->from.length = strlen(template) - 2 + email_length;
    instance->from.value = malloc(instance->from.length + 1);
    if (instance->from.value == NULL)
        return 0;

    instance->from.length = snprintf(instance->from.value, instance->from.length + 1, template, email);
    if (instance->from.length <= 0) return 0;

    return 1;
}

int __mail_set_to(mail_t* instance, const char* email) {
    if (instance == NULL) return 0;
    if (email == NULL) return 0;

    const size_t email_length = strlen(email);
    const char* template = "<%s>";

    instance->to.length = strlen(template) - 2 + email_length;
    instance->to.value = malloc(instance->to.length + 1);
    if (instance->to.value == NULL)
        return 0;

    instance->to.length = snprintf(instance->to.value, instance->to.length + 1, template, email);
    if (instance->to.length <= 0) return 0;

    return 1;
}

int __mail_set_subject(mail_t* instance, const char* subject) {
    if (instance == NULL) return 0;
    if (subject == NULL) return 0;

    size_t subject_length = strlen(subject);
    char encoded_subject[base64_encode_len(subject_length)];
    const size_t encoded_subject_length = base64_encode(encoded_subject, subject, subject_length);

    const char* template = "=?UTF-8?B?%s?=";
    instance->subject.length = strlen(template) - 2 + encoded_subject_length;
    instance->subject.value = malloc(instance->subject.length + 1);
    if (instance->subject.value == NULL)
        return 0;

    instance->subject.length = snprintf(instance->subject.value, instance->subject.length + 1, template, encoded_subject);
    if (instance->subject.length <= 0) return 0;

    return 1;
}

int __mail_set_date(mail_t* instance, time_t* rawtime) {
    if (instance == NULL) return 0;

    char timezone[7];
    {
        const int tz = timezone_offset();
        const char* sign = tz >= 0 ? "+" : "-";
        const char* zero = tz < 10 ? "0" : "";
        const int r = snprintf(timezone, sizeof(timezone), "%s%s%d00", sign, zero, tz);
        if (r <= 0) return 0;
    }

    char template[80];
    const int r = snprintf(template, sizeof(template), "%%a, %%d %%b %%Y %%T %s", timezone);
    if (r <= 0) return 0;

    struct tm* timeinfo = localtime(rawtime);

    instance->date.value = malloc(80);
    if (instance->date.value == NULL)
        return 0;

    instance->date.length = strftime(instance->date.value, 80, template, timeinfo);
    if (instance->date.length <= 0) return 0;

    return 1;
}

int __mail_set_message_id(mail_t* instance, time_t* rawtime) {
    if (instance == NULL) return 0;

    char template[80];
    const int r = snprintf(template, sizeof(template), "<%%Y%%m%%d%%H%%M%%S@%s>", env()->mail.host);
    if (r <= 0) return 0;

    struct tm* timeinfo = localtime(rawtime);

    instance->message_id.value = malloc(80);
    if (instance->message_id.value == NULL)
        return 0;

    instance->message_id.length = strftime(instance->message_id.value, 80, template, timeinfo);
    if (instance->message_id.length <= 0) return 0;

    return 1;
}

int __mail_set_content(mail_t* instance, const char* body) {
    if (instance == NULL) return 0;
    if (instance->data != NULL)
        free(instance->data);

    const int wrap = 76;
    size_t body_length = strlen(body);
    instance->data = malloc(base64_encode_nl_len(body_length, wrap));
    if (instance->data == NULL) return 0;

    instance->data_size = base64_encode_nl(instance->data, body, body_length, wrap);

    return 1;
}

int __mail_send_mail(mail_t* instance) {
    if (instance == NULL) return 0;
    
    if (!__mail_send_from(instance))
        return 0;
    if (!__mail_send_to(instance))
        return 0;
    if (!__mail_send_data(instance))
        return 0;
    if (!__mail_build_content(instance))
        return 0;
    if (!__mail_send_content(instance))
        return 0;

    return 1;
}

int __mail_send_from(mail_t* instance) {
    if (!__mail_can_interact(instance)) return 0;
    if (instance->from.length == 0) return 0;
    if (!__mail_send_command(instance, "MAIL FROM: %s\r\n", instance->from.value)) return 0;

    smtpresponse_t* response = (smtpresponse_t*)instance->connection->response;
    if (response->status != 250) {
        instance->reseted = 1;
        log_error("%s\n", response->message);
        return 0;
    }

    return 1;
}

int __mail_send_to(mail_t* instance) {
    if (!__mail_can_interact(instance)) return 0;
    if (instance->to.length == 0) return 0;
    if (!__mail_send_command(instance, "RCPT TO: %s\r\n", instance->to.value)) return 0;

    smtpresponse_t* response = (smtpresponse_t*)instance->connection->response;
    if (response->status != 250) {
        instance->reseted = 1;
        log_error("%s\n", response->message);
        return 0;
    }

    return 1;
}

int __mail_send_data(mail_t* instance) {
    if (!__mail_can_interact(instance)) return 0;
    if (!__mail_send_command(instance, "DATA\r\n")) return 0;

    smtpresponse_t* response = (smtpresponse_t*)instance->connection->response;
    if (response->status != 354) {
        instance->reseted = 1;
        log_error("%s\n", response->message);
        return 0;
    }

    protmgr_set_smtp_client_content(instance->connection);
    instance->connection->request = (request_t*)instance->request_data;

    return 1;
}

int __mail_send_content(mail_t* instance) {
    if (!__mail_can_interact(instance)) return 0;
    if (instance->data_size == 0) return 0;

    instance->connection->write(instance->connection, instance->buffer, instance->buffer_size);
    instance->connection->read(instance->connection, instance->buffer, instance->buffer_size);

    smtpresponse_t* response = (smtpresponse_t*)instance->connection->response;
    if (response->status != 250) {
        instance->reseted = 1;
        log_error("%s\n", response->message);
        return 0;
    }

    protmgr_set_smtp_client_command(instance->connection);
    instance->connection->request = (request_t*)instance->request;

    return 1;
}

int __mail_send_reset(mail_t* instance) {
    if (!__mail_can_interact(instance)) return 0;
    if (!__mail_send_command(instance, "RSET\r\n")) return 0;

    instance->reseted = 1;

    return 1;
}

void __mail_free(mail_t* instance) {
    if (instance == NULL) return;

    __mail_string_free(&instance->from_with_name);
    __mail_string_free(&instance->from);
    __mail_string_free(&instance->to);
    __mail_string_free(&instance->subject);
    __mail_string_free(&instance->date);
    __mail_string_free(&instance->message_id);
    
    if (instance->ssl_ctx != NULL) {
        SSL_CTX_free(instance->ssl_ctx);
        instance->ssl_ctx = NULL;
    }
    if (instance->connection != NULL) {
        __mail_connection_free(instance->connection);
        instance->connection = NULL;
    }
    if (instance->request != NULL) {
        instance->request->base.free(instance->request);
        instance->request = NULL;
    }
    if (instance->request_data != NULL) {
        instance->request_data->base.free(instance->request_data);
        instance->request_data = NULL;
    }
    if (instance->response != NULL) {
        instance->response->base.free(instance->response);
        instance->response = NULL;
    }
    if (instance->buffer != NULL) {
        free(instance->buffer);
        instance->buffer = NULL;
    }
    if (instance->data != NULL) {
        free(instance->data);
        instance->data = NULL;
    }

    instance->buffer_size = 0;
    instance->data_size = 0;

    mail_header_t* header = instance->_header;
    while (header) {
        mail_header_t* next = header->next;
        mail_header_free(header);
        header = next;
    }

    free(instance);
}

const char* __mail_domain_from_email(const char* email) {
    const char* domain = strchr(email, '@');
    if (domain == NULL)
        return NULL;

    domain++;

    return domain;
}

int __mail_set_conn_timeout(const int fd) {
    struct timeval timeout;      
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
        log_error("Error mail setsockopt SO_RCVTIMEO: %d\n", errno);
        return 0;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
        log_error("Error mail setsockopt SO_SNDTIMEO: %d\n", errno);
        return 0;
    }

    return 1;
}

int __mail_get_mx_servers(const char* host, mail_mx_record_t* mx_records) {
    union {
        HEADER hdr;
        unsigned char buf[NS_PACKETSZ];
    } buffer;

    const int buffer_size = res_query(host, ns_c_in, ns_t_mx, (unsigned char*)&buffer, sizeof(buffer));
    if (buffer_size == -1) {
        log_error("[getMXServers] Empty buffer: %s\n", strerror(errno));
        return 0;
    }

    if (buffer.hdr.rcode != NOERROR) {
        switch (buffer.hdr.rcode) {
            case FORMERR:
                log_error("[getMXServers] Buffer error: Format error\n");
                break;
            case SERVFAIL:
                log_error("[getMXServers] Buffer error: Server failure\n");
                break;
            case NXDOMAIN:
                log_error("[getMXServers] Buffer error: Name error\n");
                break;
            case NOTIMP:
                log_error("[getMXServers] Buffer error: Not implemented\n");
                break;
            case REFUSED:
                log_error("[getMXServers] Buffer error: Refused\n");
                break;
            default:
                log_error("[getMXServers] Buffer error: Unknown error\n");
        }

        return 0;
    }

    ns_msg message;
    if (ns_initparse(buffer.buf, buffer_size, &message) == -1) {
        log_error("[getMXServers] Can't init parse ns: %s\n", strerror(errno));
        return 0;
    }

    ns_rr resource_record;
    if (ns_parserr(&message, ns_s_qd, 0, &resource_record) == -1) {
        log_error("[getMXServers] Can't parse question section: %s\n", strerror(errno));
        return 0;
    }

    int result = 0;
    int answers = ntohs(buffer.hdr.ancount);
    answers = answers > MAIL_RECORDS_SIZE ? MAIL_RECORDS_SIZE : answers;
    for (int i = 0; i < answers; ++i)
        if (__mail_parse_mx_record(buffer.buf, buffer_size, ns_s_an, i, &message, &mx_records[i]))
            result = 1;

    return result;
}

int __mail_parse_mx_record(unsigned char* buffer, size_t r, ns_sect s, int idx, ns_msg* message, mail_mx_record_t* mx_record) {
    ns_rr resource_record;
    if (ns_parserr(message, s, idx, &resource_record) == -1) {
        log_error("[parseMxRecord] Can't parse answer section: %s\n", strerror(errno));
        return 0;
    }

    if (ns_rr_type(resource_record) != ns_t_mx)
        return 0;

    const unsigned char* data = ns_rr_rdata(resource_record);
    mx_record->preference = ns_get16(data);

    {
        unsigned char tmpname[NS_MAXDNAME];
        ns_name_unpack(buffer, buffer + r, data + sizeof(u_int16_t), tmpname, NS_MAXDNAME);
        ns_name_ntop(tmpname, mx_record->domain, NS_MAXDNAME);
    }

    struct hostent* he = gethostbyname(mx_record->domain);
    if (he == NULL) {
        log_error("[parseMxRecord] Error get host by name\n");
        return 0;
    }

    mx_record->ok = 1;

    struct in_addr** addr_list = (struct in_addr**)he->h_addr_list;
    for (int i = 0; addr_list[i] != NULL && i < MAIL_IP_LIST_SIZE; i++) {
        log_info("%d\n", *addr_list[i]);

        memcpy(&mx_record->ip_list[i], addr_list[i], sizeof(struct in_addr));
    }

    log_info("[parseMxRecord] pref: %d, host name: %s\n", mx_record->preference, mx_record->domain);

    return 1;
}

int __mail_can_interact(mail_t* instance) {
    if (instance == NULL) return 0;

    if (!__mail_connected(instance)) {
        log_error("[__mail_can_interact] Not connected\n");
        return 0;
    }

    if (instance->reseted) {
        log_error("[__mail_can_interact] Reseted\n");
        return 0;
    }

    return 1;
}

int __mail_send_command(mail_t* instance, const char* format, ...) {
    if (instance == NULL) return 0;
    if (format == NULL) return 0;

    va_list args;
    va_start(args, format);
    size_t size = vsnprintf(instance->request->command, sizeof(instance->request->command), format, args);
    va_end(args);

    if (size <= 2) return 0;
    if (instance->request->command[size - 2] != '\r' && instance->request->command[size - 1] != '\n') return 0;

    log_info("%s\n", instance->request->command);

    instance->connection->write(instance->connection, instance->buffer, instance->buffer_size);
    instance->connection->read(instance->connection, instance->buffer, instance->buffer_size);

    return 1;
}

int __mail_init_tls(mail_t* instance) {
    if (instance == NULL) return 0;

    int result = 0;

    instance->ssl_ctx = SSL_CTX_new(TLS_method());
    if (instance->ssl_ctx == NULL) goto failed;

    instance->connection->ssl_ctx = instance->ssl_ctx;
    instance->connection->port = 587;

    if (!__mail_alloc_ssl(instance->connection))
        goto failed;

    if (!__mail_handshake(instance->connection))
        goto failed;

    result = 1;

    failed:

    return result;
}

int __mail_alloc_ssl(connection_t* connection) {
    if (connection->ssl != NULL) return 1;

    int result = 0;

    connection->ssl = SSL_new(connection->ssl_ctx);
    if (connection->ssl == NULL)
        goto failed;

    if (!SSL_set_fd(connection->ssl, connection->fd))
        goto failed;

    SSL_set_connect_state(connection->ssl);

    result = 1;

    failed:

    if (result == 0) {
        if (connection->ssl) {
            SSL_free(connection->ssl);
            connection->ssl = NULL;
        }
    }

    return result;
}

int __mail_handshake(connection_t* connection) {
    protmgr_set_smtp_client_tls(connection);
    connection->write(connection, NULL, 0);

    if (connection->request == NULL || connection->fd == 0)
        return 0;

    return 1;
}

int __mail_set_dkim_headers(dkim_t* dkim, mail_t* instance) {
    if (!dkim_header_add(dkim, "From", 4, instance->from_with_name.value, instance->from_with_name.length)) return 0;
    if (!dkim_header_add(dkim, "To", 2, instance->to.value, instance->to.length)) return 0;
    if (!dkim_header_add(dkim, "Subject", 7, instance->subject.value, instance->subject.length)) return 0;
    if (!dkim_header_add(dkim, "Date", 4, instance->date.value, instance->date.length)) return 0;
    if (!dkim_header_add(dkim, "Message-Id", 10, instance->message_id.value, instance->message_id.length)) return 0;

    return 1;
}

int __mail_build_content(mail_t* instance) {
    if (instance == NULL) return 0;

    int result = 0;
    time_t rawtime = time(0);
    dkim_t* dkim = NULL;
    char* dkim_sign = NULL;

    if (!__mail_set_date(instance, &rawtime))
        goto failed;
    if (!__mail_set_message_id(instance, &rawtime))
        goto failed;

    dkim = dkim_create();
    if (dkim == NULL)
        goto failed;

    dkim_set_private_key(dkim, env()->mail.dkim_private);
    dkim_set_domain(dkim, env()->mail.host);
    dkim_set_selector(dkim, env()->mail.dkim_selector);
    dkim_set_timestamp(dkim, rawtime);

    if (!__mail_set_dkim_headers(dkim, instance))
        goto failed;

    dkim_sign = dkim_create_sign(dkim, instance->data);
    if (dkim_sign == NULL)
        goto failed;

    if (!__mail_header_add(instance, "From", instance->from_with_name.value)) goto failed;
    if (!__mail_header_add(instance, "To", instance->to.value)) goto failed;
    if (!__mail_header_add(instance, "Subject", instance->subject.value)) goto failed;
    if (!__mail_header_add(instance, "Date", instance->date.value)) goto failed;
    if (!__mail_header_add(instance, "Message-Id", instance->message_id.value)) goto failed;
    if (!__mail_header_add(instance, "DKIM-Signature", dkim_sign)) goto failed;
    if (!__mail_header_add(instance, "MIME-Version", "1.0")) goto failed;
    if (!__mail_header_add(instance, "Content-Transfer-Encoding", "base64")) goto failed;
    if (!__mail_header_add(instance, "Content-Type", "text/html; charset=utf-8")) goto failed;

    instance->request_data->content_size = __mail_calc_content_length(instance);
    instance->request_data->content = malloc(sizeof(char) * instance->request_data->content_size);
    if (instance->request_data->content == NULL)
        goto failed;

    size_t pos = 0;
    mail_header_t* header = instance->_header;
    while (header) {
        if (!__mail_data_append(instance->request_data->content, &pos, header->key, header->key_length)) goto failed;
        if (!__mail_data_append(instance->request_data->content, &pos, ": ", 2)) goto failed;
        if (!__mail_data_append(instance->request_data->content, &pos, header->value, header->value_length)) goto failed;
        if (!__mail_data_append(instance->request_data->content, &pos, "\r\n", 2)) goto failed;

        header = header->next;
    }

    if (!__mail_data_append(instance->request_data->content, &pos, "\r\n", 2)) goto failed;
    if (!__mail_data_append(instance->request_data->content, &pos, instance->data, instance->data_size)) goto failed;
    if (!__mail_data_append(instance->request_data->content, &pos, "\r\n.\r\n", 5)) goto failed;

    result = 1;

    failed:

    if (result == 0) {
        instance->reseted = 1;
    }

    if (dkim != NULL) dkim_free(dkim);
    if (dkim_sign != NULL) free(dkim_sign);

    return result;
}

int __mail_header_add(mail_t* instance, const char* key, const char* value) {
    if (instance == NULL) return 0;
    if (key == NULL) return 0;
    if (value == NULL) return 0;
    if (key[0] == 0) return 0;
    if (value[0] == 0) return 0;

    const size_t key_length = strlen(key);
    const size_t value_length = strlen(value);
    mail_header_t* header = mail_header_create(key, key_length, value, value_length);
    if (header == NULL) return 0;
    if (header->key == NULL || header->value == NULL) {
        mail_header_free(header);
        return 0;
    }

    if (instance->_header == NULL)
        instance->_header = header;

    if (instance->_last_header != NULL)
        instance->_last_header->next = header;

    instance->_last_header = header;

    return 1;
}

size_t __mail_calc_content_length(mail_t* instance) {
    mail_header_t* header = instance->_header;
    size_t size = 0;

    while (header) {
        size += header->key_length;
        size += 2; // ": "
        size += header->value_length;
        size += 2; // "\r\n"

        header = header->next;
    }

    size += 2; // "\r\n"
    size += instance->data_size;
    size += 5; // "\r\n.\r\n"

    return size;
}

int __mail_data_append(char* data, size_t* pos, const char* string, const size_t length) {
    memcpy(&data[*pos], string, length);
    *pos += length;

    return 1;
}

int __mail_after_read(connection_t* connection) {
    (void)connection;
    return 0;
}

int __mail_after_write(connection_t* connection) {
    (void)connection;
    return 0;
}