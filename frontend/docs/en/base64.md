---
outline: deep
description: Base64 library in C Web Framework. Encoding and decoding data in Base64 format.
---

# Base64 library

Library for encoding and decoding data in Base64 format.

## Encoding

### Calculating encoded string length

```c
#include "base64.h"

int base64_encode_inline_len(int len);
```

Calculates the length of the encoded string based on the source length.

**Parameters**\
`len` — source string length.

**Return value**\
Encoded string length.

<br>

### Encoding a string

```c
int base64_encode_inline(char* coded_dst, const char* plain_src, int len_plain_src);
```

Encodes string `plain_src` to Base64.

**Parameters**\
`coded_dst` — pointer to the buffer for writing the encoded string.\
`plain_src` — pointer to the source string.\
`len_plain_src` — source string length.

**Return value**\
Encoded string length.

## Decoding

### Calculating decoded string length

```c
int base64_decode_inline_len(const char* coded_src);
```

Calculates the length of the source string based on the encoded length.

**Parameters**\
`coded_src` — pointer to the encoded string.

**Return value**\
Decoded string length.

<br>

### Decoding a string

```c
int base64_decode_inline(char* plain_dst, const char* coded_src);
```

Decodes string `coded_src` from Base64.

**Parameters**\
`plain_dst` — pointer to the buffer for writing the decoded string.\
`coded_src` — pointer to the encoded string.

**Return value**\
Decoded string length.

## Usage examples

### Encoding data

```c
#include "base64.h"
#include <stdlib.h>
#include <string.h>

void encode_example(httpctx_t* ctx) {
    const char* original = "Hello, World!";
    int original_len = strlen(original);

    // Calculate buffer size for result
    int encoded_len = base64_encode_inline_len(original_len);
    char* encoded = malloc(encoded_len + 1);

    if (encoded == NULL) {
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    // Encode
    int result_len = base64_encode_inline(encoded, original, original_len);
    encoded[result_len] = '\0';

    // encoded = "SGVsbG8sIFdvcmxkIQ=="
    ctx->response->send_data(ctx->response, encoded);

    free(encoded);
}
```

### Decoding data

```c
void decode_example(httpctx_t* ctx) {
    const char* encoded = "SGVsbG8sIFdvcmxkIQ==";

    // Calculate buffer size for result
    int decoded_len = base64_decode_inline_len(encoded);
    char* decoded = malloc(decoded_len + 1);

    if (decoded == NULL) {
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    // Decode
    int result_len = base64_decode_inline(decoded, encoded);
    decoded[result_len] = '\0';

    // decoded = "Hello, World!"
    ctx->response->send_data(ctx->response, decoded);

    free(decoded);
}
```

### Encoding binary data

```c
void encode_binary(httpctx_t* ctx) {
    // Binary data (e.g., image)
    unsigned char binary_data[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
    int data_len = sizeof(binary_data);

    int encoded_len = base64_encode_inline_len(data_len);
    char* encoded = malloc(encoded_len + 1);

    if (encoded == NULL) {
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    int result_len = base64_encode_inline(encoded, (const char*)binary_data, data_len);
    encoded[result_len] = '\0';

    ctx->response->send_data(ctx->response, encoded);

    free(encoded);
}
```

### Using in API

```c
void get_file_base64(httpctx_t* ctx) {
    // Get file from storage
    file_t file = storage_file_get("local", "images/logo.png");
    if (!file.ok) {
        ctx->response->statusCode = 404;
        ctx->response->send_data(ctx->response, "File not found");
        return;
    }

    // Read file content
    char* content = malloc(file.size);
    if (content == NULL) {
        file.close(&file);
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    // ... read file into content ...

    // Encode to Base64
    int encoded_len = base64_encode_inline_len(file.size);
    char* encoded = malloc(encoded_len + 1);

    if (encoded == NULL) {
        free(content);
        file.close(&file);
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    int result_len = base64_encode_inline(encoded, content, file.size);
    encoded[result_len] = '\0';

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

    int encoded_len = base64_encode_inline_len(image_size);
    // +30 for prefix "data:image/png;base64,"
    char* data_url = malloc(encoded_len + 30);

    if (data_url == NULL) {
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    // Form Data URL
    strcpy(data_url, "data:image/png;base64,");
    int offset = strlen(data_url);

    base64_encode_inline(data_url + offset, image_data, image_size);

    ctx->response->send_data(ctx->response, data_url);

    free(data_url);
}
```
