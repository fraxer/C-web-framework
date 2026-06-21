---
outline: deep
description: Base64 library in C Web Framework. Encoding and decoding data in Base64 format.
---

# Base64 library

Library for encoding and decoding data to/from Base64 (RFC 4648). The alphabet is the standard `A–Z`, `a–z`, `0–9`, `+`, `/`, with `=` padding.

::: tip Null terminator
All encoding and decoding functions **null-terminate the result**. The `*_len` functions return the required buffer size **including** the null terminator — so `malloc(base64_*_len(...))` allocates exactly as much memory as needed. The `encode`/`decode` functions themselves return the length of the payload **excluding** the null terminator.
:::

## Encoding

### Calculating buffer size

```c
#include "base64.h"

int base64_encode_len(const int len);
```

Returns the buffer size required to encode `len` bytes, including the null terminator.

**Parameters**\
`len` — length of the source data.

**Return value**\
Buffer size in bytes (including `\0`).

<br>

### Encoding

```c
int base64_encode(char* encoded, const char* string, const int string_len);
```

Encodes `string_len` bytes from `string` into Base64 and appends a null terminator.

**Parameters**\
`encoded` — destination buffer (at least `base64_encode_len(string_len)` bytes).\
`string` — source data.\
`string_len` — length of the source data.

**Return value**\
Length of the encoded string **excluding** the null terminator.

<br>

### Encoding with line wrapping

```c
int base64_encode_nl_len(const int len, int wrap);
int base64_encode_nl(char* encoded, const char* string, const int string_len, const int wrap);
```

Encodes data, inserting `\r\n` every `wrap` characters of output — the format used by PEM and MIME (RFC 2045; a typical line width is 76 characters). With `wrap == 0` it behaves like plain encoding.

**Parameters**\
`len` — length of the source data (for `*_len`).\
`encoded` — destination buffer.\
`string` — source data.\
`string_len` — length of the source data.\
`wrap` — line width in characters; `0` disables wrapping.

**Return value**\
`base64_encode_nl_len` — buffer size including `\0` and the line-break characters.\
`base64_encode_nl` — length of the encoded data **excluding** the null terminator.

## Decoding

### Calculating buffer size

```c
int base64_decode_len(const char* bufcoded);
```

Returns an upper bound for the decoded size based on the encoded string, including the null terminator. `\r` and `\n` in `bufcoded` are skipped, so the input may contain line breaks.

**Parameters**\
`bufcoded` — encoded string (null-terminated).

**Return value**\
Buffer size in bytes (including `\0`).

<br>

### Decoding

```c
int base64_decode(char* bufplain, const char* bufcoded);
```

Decodes `bufcoded` from Base64 and appends a null terminator. `\r` and `\n` characters in the input are ignored.

**Parameters**\
`bufplain` — destination buffer (at least `base64_decode_len(bufcoded)` bytes).\
`bufcoded` — encoded string (null-terminated).

**Return value**\
Length of the decoded data **excluding** the null terminator.

## Usage examples

### Encoding data

```c
#include "http.h"
#include "base64.h"
#include <stdlib.h>
#include <string.h>

void encode_example(httpctx_t* ctx) {
    const char* original = "Hello, World!";
    int original_len = strlen(original);

    // base64_encode_len already accounts for the null terminator
    char* encoded = malloc(base64_encode_len(original_len));
    if (encoded == NULL) {
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    // Encode — the result is null-terminated
    base64_encode(encoded, original, original_len);

    // encoded = "SGVsbG8sIFdvcmxkIQ=="
    ctx->response->send_data(ctx->response, encoded);

    free(encoded);
}
```

### Decoding data

```c
void decode_example(httpctx_t* ctx) {
    const char* encoded = "SGVsbG8sIFdvcmxkIQ==";

    // base64_decode_len already accounts for the null terminator
    char* decoded = malloc(base64_decode_len(encoded));
    if (decoded == NULL) {
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    // Decode — the result is null-terminated
    base64_decode(decoded, encoded);

    // decoded = "Hello, World!"
    ctx->response->send_data(ctx->response, decoded);

    free(decoded);
}
```

### Encoding binary data

```c
void encode_binary(httpctx_t* ctx) {
    // Binary data (e.g. a PNG signature)
    unsigned char binary_data[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
    int data_len = sizeof(binary_data);

    char* encoded = malloc(base64_encode_len(data_len));
    if (encoded == NULL) {
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    base64_encode(encoded, (const char*)binary_data, data_len);

    ctx->response->send_data(ctx->response, encoded);

    free(encoded);
}
```

### Encoding with line wrapping (PEM/MIME)

```c
void encode_pem(httpctx_t* ctx) {
    const char* original = "Very long payload ...";
    int original_len = strlen(original);

    // 76 is the standard PEM/MIME line width
    char* encoded = malloc(base64_encode_nl_len(original_len, 76));
    if (encoded == NULL) {
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    base64_encode_nl(encoded, original, original_len, 76);

    ctx->response->send_data(ctx->response, encoded);

    free(encoded);
}
```

### Using in API

```c
#include "http.h"
#include "base64.h"
#include "storage.h"
#include <stdlib.h>

void get_file_base64(httpctx_t* ctx) {
    // Get file from storage
    file_t file = storage_file_get("local", "images/logo.png");
    if (!file.ok) {
        ctx->response->status_code = 404;
        ctx->response->send_data(ctx->response, "File not found");
        return;
    }

    // Read file content
    char* content = file.content(&file);
    if (content == NULL) {
        file.close(&file);
        ctx->response->send_data(ctx->response, "Read error");
        return;
    }

    // Encode to Base64
    char* encoded = malloc(base64_encode_len(file.size));
    if (encoded == NULL) {
        free(content);
        file.close(&file);
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    base64_encode(encoded, content, file.size);

    // Form JSON response
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);
    json_object_set(root, "filename", json_create_string(file.name));
    json_object_set(root, "data", json_create_string(encoded));

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(doc));

    json_free(doc);
    free(encoded);
    free(content);
    file.close(&file);
}
```

### Data URL for images

```c
void get_image_data_url(httpctx_t* ctx) {
    const char* image_data = "..."; // binary image data
    int image_size = 1024;          // data size

    const char* prefix = "data:image/png;base64,";
    // base64_encode_len already accounts for the null terminator; +strlen(prefix) for the prefix
    char* data_url = malloc(base64_encode_len(image_size) + strlen(prefix));
    if (data_url == NULL) {
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    // Write the prefix, then encode the data right after it
    strcpy(data_url, prefix);
    base64_encode(data_url + strlen(prefix), image_data, image_size);

    ctx->response->send_data(ctx->response, data_url);

    free(data_url);
}
```
