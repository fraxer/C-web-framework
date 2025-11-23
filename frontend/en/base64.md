---
outline: deep
description: Description of the base64 method library
---

# Base64 library

```C
int base64_encode_inline_len(int len);
```

Calculation of the encoded string length based on the source length.

**Parameters**\
`len` - source string length.

**Return value**\
Encoded string length.

<br>

```C
int base64_encode_inline(char* coded_dst, const char* plain_src, int len_plain_src);
```

Encode string `plain_src`. 

**Parameters**\
`coded_dst` - pointer to the character array to write to.\
`plain_src` - pointer to the byte string to copy from.\
`len_plain_src` - source string length.

**Return value**\
Encoded string length.

<br>

```C
int base64_decode_inline_len(const char* coded_src);
```

Calculation of the source string length based on the encoded length.

**Parameters**\
`coded_src` - pointer to the encoded byte string to copy from.

**Return value**\
Decoded string length.

<br>

```C
int base64_decode_inline(char* plain_dst, const char* coded_src);
```

Decode string `coded_src`.

**Parameters**\
`plain_dst` - pointer to the character array to write to.\
`coded_src` - pointer to the byte string to copy from.

**Return value**\
Decoded string length.
