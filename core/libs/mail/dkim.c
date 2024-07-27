#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/engine.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/buffer.h>
#include <openssl/err.h>

#include "log.h"
#include "dkim.h"
#include "dkimcanonparser.h"
#include "dkimheaderparser.h"
#include "base64.h"

int __dkim_relaxed_header_canon(dkim_t* dkim);
char* __dkim_relaxed_body_canon(const char *body);
char* __dkim_wrap(char *str, size_t len);
RSA* __dkim_rsa_read_pem(const char* buffer);
char* __dkim_header_list_create(dkim_t* dkim, int* header_list_length);
int __dkim_header_count(mail_header_t* header);
int __dkim_header_keys_length(mail_header_t* header);
char* __dkim_base64_encode_sha1(const char* body, int* canon_body_length);
char* __dkim_sign_create(unsigned char* hash, const char* private_key, int* sign64_length);
char* __dkim_make_headers_string(dkim_t* dkim, int* headers_string_length);
char* __dkim_add_sign_to_dkim(const char* dkim, size_t dkim_length, const char* sign, size_t sign_length);

/* The "relaxed" Header Canonicalization Algorithm */
int __dkim_relaxed_header_canon(dkim_t* dkim) {
    int result = 0;
    dkimheaderparser_t* parser = dkimheaderparser_alloc();
    if (parser == NULL) return 0;

    dkimheaderparser_init(parser);

    /* Copy all headers */
    /* Convert all header field names (not the header field values) to
       lowercase.  For example, convert "SUBJect: AbC" to "subject: AbC". */
    /* Convert all sequences of one or more WSP characters to a single SP
       character. WSP characters here include those before and after a
       line folding boundary. */
    mail_header_t* header = dkim->header;
    while (header) {
        for (size_t e = 0; e < header->key_length; ++e)
            header->key[e] = tolower(header->key[e]);

        dkimheaderparser_set_buffer(parser, header->value, header->value_length);
        if (!dkimheaderparser_run(parser))
            goto failed;

        char* new_body = dkimheaderparser_get_content(parser);
        if (new_body == NULL)
            goto failed;

        free(header->value);
        header->value = new_body;
        header->value_length = dkimheaderparser_get_content_length(parser);

        header = header->next;
    }

    result = 1;

    failed:

    dkimheaderparser_free(parser);

    return result;
}

/* "relaxed" body canonicalization */
char* __dkim_relaxed_body_canon(const char *body) {
    dkimcanonparser_t* parser = dkimcanonparser_alloc();
    if (parser == NULL) return NULL;

    dkimcanonparser_init(parser);
    dkimcanonparser_set_buffer(parser, body, strlen(body));
    const int success = dkimcanonparser_run(parser);
    char* new_body = success ? dkimcanonparser_get_content(parser) : NULL;

    dkimcanonparser_free(parser);

    return new_body;
}

char* __dkim_wrap(char* str, size_t len) {
    size_t count_newlines = 0;
    int lcount = 0;
    for (size_t i = 0; i < len; ++i) {
        if (str[i] == ' ' || lcount == 75) {
            count_newlines++;
            lcount = 0;
        }
        else
            ++lcount;
    }

    lcount = 0;
    char* tmp = malloc(len + (3 * count_newlines) + 1);
    if (tmp == NULL)
        return NULL;

    size_t tmp_len = 0;
    for (size_t i = 0; i < len; ++i) {
        if (str[i] == ' ' || lcount == 75) {
            tmp[tmp_len++] = str[i];
            tmp[tmp_len++] = '\r';
            tmp[tmp_len++] = '\n';
            tmp[tmp_len++] = '\t';
            lcount = 0;
        } else {
            tmp[tmp_len++] = str[i];
            ++lcount;
        }
    }

    tmp[tmp_len] = 0;

    return tmp;
}

RSA* __dkim_rsa_read_pem(const char* buffer) {
    BIO* mem = BIO_new_mem_buf(buffer, strlen(buffer));
    if (mem == NULL) return NULL;

    RSA* rsa = PEM_read_bio_RSAPrivateKey(mem, NULL, NULL, NULL);
    BIO_free(mem);

    return rsa;
} 

/* h1:h2:h3 */
char* __dkim_header_list_create(dkim_t* dkim, int* header_list_length) {
    *header_list_length = __dkim_header_keys_length(dkim->header) + __dkim_header_count(dkim->header) - 1;
    char* header_list = malloc(sizeof(char) * ((*header_list_length) + 1));
    if (header_list == NULL) {
        *header_list_length = 0;
        return NULL;
    }

    int pos = 0;
    mail_header_t* header = dkim->header;
    while (header) {
        if (pos != 0)
            pos += sprintf(header_list + pos, ":");

        pos += sprintf(header_list + pos, "%s", header->key);

        header = header->next;
    }

    return header_list;
}

int __dkim_header_count(mail_header_t* header) {
    int count = 0;
    while (header) {
        count++;
        header = header->next;
    }

    return count;
}

int __dkim_header_keys_length(mail_header_t* header) {
    int length = 0;
    while (header) {
        length += header->key_length;
        header = header->next;
    }

    return length;
}

char* __dkim_base64_encode_sha1(const char* body, int* canon_body_length) {
    char* canon_body = __dkim_relaxed_body_canon(body);
    if (canon_body == NULL) return NULL;

    *canon_body_length = strlen(canon_body);

    unsigned char uhash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)canon_body, *canon_body_length, uhash);
    free(canon_body);

    char* base64string = malloc(base64_encode_len(SHA_DIGEST_LENGTH));
    if (base64string == NULL)
        return NULL;

    if (!base64_encode(base64string, (const char*)uhash, SHA_DIGEST_LENGTH)) {
        *canon_body_length = 0;
        free(base64string);
        return NULL;
    }

    return base64string;
}

char* __dkim_sign_create(unsigned char* hash, const char* private_key, int* sign64_length) {
    RSA* rsa_private = __dkim_rsa_read_pem(private_key);
    if (rsa_private == NULL) {
        log_error("DKIM error loading rsa key\n");
        return NULL;
    }

    char* result = NULL;
    char* base64sign = NULL;
    unsigned int sign_length = 0;
    char* sign = malloc(RSA_size(rsa_private));
    if (sign == NULL)
        goto failed;

    if (!RSA_sign(NID_sha1, hash, SHA_DIGEST_LENGTH, (unsigned char*)sign, (unsigned int*)&sign_length, rsa_private)) {
        log_error("DKIM error rsa sign: %s\n", ERR_error_string(ERR_get_error(), NULL));
        goto failed;
    }

    base64sign = malloc(base64_encode_len(sign_length));
    if (base64sign == NULL)
        goto failed;

    *sign64_length = base64_encode(base64sign, sign, sign_length);
    if (*sign64_length == 0) {
        free(base64sign);
        goto failed;
    }

    result = base64sign;

    failed:

    if (rsa_private != NULL)
        free(rsa_private);
    if (sign != NULL)
        free(sign);

    return result;
}

char* __dkim_make_headers_string(dkim_t* dkim, int* headers_string_length) {
    *headers_string_length = 0;

    mail_header_t* header = dkim->header;
    while (header) {
        *headers_string_length += header->key_length + header->value_length;
        *headers_string_length += header->next != NULL ? 3 : 1;

        header = header->next;
    }

    char* string = malloc(*headers_string_length + 1);
    if (string == NULL) {
        *headers_string_length = 0;
        return NULL;
    }

    size_t offset = 0;
    header = dkim->header;
    while (header) {
        const char* template = header->next != NULL ? "%s:%s\r\n" : "%s:%s";
        offset += snprintf(string + offset, *headers_string_length, template, header->key, header->value);

        header = header->next;
    }

    return string;
}

char* __dkim_add_sign_to_dkim(const char* dkim, size_t dkim_length, const char* sign, size_t sign_length) {
    const size_t extra_dkim_length = dkim_length + sign_length;
    char* extra_dkim = malloc(extra_dkim_length + 1);
    if (extra_dkim == NULL)
        return NULL;

    memcpy(extra_dkim, dkim, dkim_length);
    memcpy(extra_dkim + dkim_length, sign, sign_length + 1);

    char* wrap_dkim = __dkim_wrap(extra_dkim, extra_dkim_length);

    free(extra_dkim);

    return wrap_dkim;
}

dkim_t* dkim_create() {
    dkim_t* dkim = malloc(sizeof * dkim);
    if (dkim == NULL) return NULL;

    dkim->private_key = NULL;
    dkim->domain = NULL;
    dkim->selector = NULL;
    dkim->timestamp = 0;
    dkim->header = NULL;
    dkim->last_header = NULL;

    return dkim;
}

int dkim_header_add(dkim_t* dkim, const char* key, const size_t key_length, const char* value, const size_t value_length) {
    if (dkim == NULL) return 0;
    if (key == NULL) return 0;
    if (value == NULL) return 0;
    if (key[0] == 0) return 0;
    if (value[0] == 0) return 0;

    mail_header_t* header = mail_header_create(key, key_length, value, value_length);
    if (header == NULL) return 0;
    if (header->key == NULL || header->value == NULL) {
        mail_header_free(header);
        return 0;
    }

    if (dkim->header == NULL)
        dkim->header = header;

    if (dkim->last_header != NULL)
        dkim->last_header->next = header;

    dkim->last_header = header;

    return 1;
}

void dkim_set_private_key(dkim_t* dkim, const char* private_key) {
    dkim->private_key = private_key;
}

void dkim_set_domain(dkim_t* dkim, const char* domain) {
    dkim->domain = domain;
}

void dkim_set_selector(dkim_t* dkim, const char* selector) {
    dkim->selector = selector;
}

void dkim_set_timestamp(dkim_t* dkim, const time_t timestamp) {
    dkim->timestamp = timestamp;
}

/**
 * http://tools.ietf.org/html/rfc4871
 */
char* dkim_create_sign(dkim_t* dkim, const char* body) {
    if (!__dkim_relaxed_header_canon(dkim))
        return NULL;

    char* data = NULL;
    char* headers_string = NULL;
    char* sign = NULL;
    char* full_dkim = NULL;
    int canon_body_length = 0;
    char* base64_hash = __dkim_base64_encode_sha1(body, &canon_body_length);
    if (base64_hash == NULL)
        return NULL;

    int header_list_length = 0;
    char* header_list = __dkim_header_list_create(dkim, &header_list_length);
    if (header_list == NULL)
        goto failed;

    /* create DKIM header */
    const char* template = "v=1; a=rsa-sha1; s=%s; d=%s; l=%d; t=%d; c=relaxed/relaxed; h=%s; bh=%s; b=";
    data = malloc(strlen(template) + strlen(dkim->selector) + strlen(dkim->domain) + 16 + 16 + header_list_length + strlen(base64_hash) + 1);
    if (data == NULL)
        goto failed;

    const size_t data_length = sprintf(data, template, dkim->selector, dkim->domain, canon_body_length, dkim->timestamp, header_list, base64_hash);

    const char* h_dkim_sign = "dkim-signature";
    dkim_header_add(dkim, h_dkim_sign, strlen(h_dkim_sign), data, data_length);

    int headers_string_length = 0;
    headers_string = __dkim_make_headers_string(dkim, &headers_string_length);
    if (headers_string == NULL)
        goto failed;

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)headers_string, headers_string_length, hash);

    int sign_length = 0;
    sign = __dkim_sign_create(hash, dkim->private_key, &sign_length);
    if (sign == NULL)
        goto failed;

    full_dkim = __dkim_add_sign_to_dkim(data, data_length, sign, sign_length);
    if (full_dkim == NULL)
        goto failed;

    failed:

    if (base64_hash != NULL) free(base64_hash);
    if (header_list != NULL)free(header_list);
    if (data != NULL) free(data);
    if (headers_string != NULL) free(headers_string);
    if (sign != NULL) free(sign);

    return full_dkim;
}

void dkim_free(dkim_t* dkim) {
    mail_header_t* header = dkim->header;
    while (header) {
        mail_header_t* next = header->next;
        mail_header_free(header);
        header = next;
    }

    free(dkim);
}
