#include <string.h>

#include "base64.h"

static const unsigned char pr2six[256] = {
    /* ASCII table */
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
    64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

static const char basis_64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int __base64_encode_intenal_len(const int len, const int wrap);
int __base64_encode_internal(char* encoded, const char* string, const int string_len, const int wrap);

int base64_encode_len(const int len) {
    const int wrap = 0;

    return __base64_encode_intenal_len(len, wrap);
}

int base64_encode(char* encoded, const char* string, const int string_len) {
    const int wrap = 0;

    return __base64_encode_internal(encoded, string, string_len, wrap);
}

int base64_encode_nl_len(const int len, const int wrap) {
    return __base64_encode_intenal_len(len, wrap);
}

int base64_encode_nl(char* encoded, const char* string, const int string_len, const int wrap) {
    return __base64_encode_internal(encoded, string, string_len, wrap);
}

int base64_decode_len(const char* bufcoded) {
    register int nprbytes = 0;
    register const unsigned char* bufin = (const unsigned char*)bufcoded;

    while (1) {
        if (*bufin == '\r' || *bufin == '\n') {
            bufin++;
            continue;
        }
        if (pr2six[*bufin++] > 63)
            break;

        nprbytes++;
    }

    return (((nprbytes + 3) / 4) * 3) + 1;
}

int base64_decode(char* bufplain, const char* bufcoded) {
    register int nprbytes = 0;
    register const unsigned char* bufin = (const unsigned char*)bufcoded;

    while (1) {
        if (*bufin == '\r' || *bufin == '\n') {
            bufin++;
            continue;
        }
        if (pr2six[*bufin++] > 63)
            break;

        nprbytes++;
    }

    int nbytesdecoded = ((nprbytes + 3) / 4) * 3;

    register unsigned char* bufout = (unsigned char*)bufplain;
    bufin = (const unsigned char*)bufcoded;

    while (nprbytes > 4) {
        if (*bufin == '\r' || *bufin == '\n') {
            bufin++;
            continue;
        }

        *(bufout++) = (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
        *(bufout++) = (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
        *(bufout++) = (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);

        bufin += 4;
        nprbytes -= 4;
    }

    /* Note: (nprbytes == 1) would be an error, so just ingore that case */
    if (nprbytes > 1)
        *(bufout++) = (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);

    if (nprbytes > 2)
        *(bufout++) = (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);

    if (nprbytes > 3)
        *(bufout++) = (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);

    *(bufout++) = '\0';
    nbytesdecoded -= (4 - nprbytes) & 3;

    return nbytesdecoded;
}

int __base64_encode_intenal_len(const int len, const int wrap) {
    const int inline_len = ((len + 2) / 3 * 4) + 1;

    if (wrap > 0) {
        int nl = (inline_len / wrap) * 2;
        if  (nl > 1 && inline_len % wrap == 0)
            nl -= 2;

        return inline_len + nl;
    }

    return inline_len;
}

int __base64_encode_internal(char* encoded, const char* string, const int string_len, const int wrap) {
    int i;
    int pc = 0;
    char* p = encoded;

    for (i = 0; i < string_len - 2; i += 3) {
        *p++ = basis_64[(string[i] >> 2) & 0x3F];
        *p++ = basis_64[((string[i] & 0x3) << 4) |
                        ((int) (string[i + 1] & 0xF0) >> 4)];
        *p++ = basis_64[((string[i + 1] & 0xF) << 2) |
                        ((int) (string[i + 2] & 0xC0) >> 6)];
        *p++ = basis_64[string[i + 2] & 0x3F];

        if (wrap > 0) {
            pc += 4;
            if (pc % 64 == 0) {
                *p++ = '\r';
                *p++ = '\n';
            }
        }
    }

    if (i < string_len) {
        *p++ = basis_64[(string[i] >> 2) & 0x3F];
        if (i == (string_len - 1)) {
            *p++ = basis_64[((string[i] & 0x3) << 4)];
            *p++ = '=';
        }
        else {
            *p++ = basis_64[((string[i] & 0x3) << 4) |
                            ((int) (string[i + 1] & 0xF0) >> 4)];
            *p++ = basis_64[((string[i + 1] & 0xF) << 2)];
        }
        *p++ = '=';
    }

    *p++ = '\0';

    return (p - encoded) - 1;
}
