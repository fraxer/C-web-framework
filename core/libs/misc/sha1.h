#ifndef SHA1_H
#define SHA1_H

#include <stddef.h>

#define SHA1_BLOCK_SIZE 20

typedef unsigned char BYTE;
typedef unsigned int  WORD;

typedef struct {
	BYTE data[64];
	WORD datalen;
	unsigned long long bitlen;
	WORD state[5];
	WORD k[4];
} SHA1_CTX;


void sha1(const unsigned char* data, size_t size, unsigned char* result);

#endif
