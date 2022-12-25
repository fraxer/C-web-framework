#ifndef __BASE64__
#define __BASE64__

int base64_encode_inline_len(int len);
int base64_encode_inline(char * coded_dst, const char *plain_src, int len_plain_src);

int base64_decode_inline_len(const char * coded_src);
int base64_decode_inline(char * plain_dst, const char *coded_src);

#endif