---
outline: deep
description: Библиотека Base64 в C Web Framework. Кодирование и декодирование данных в формат Base64.
---

# Библиотека Base64

Библиотека для кодирования и декодирования данных в формат Base64.

## Кодирование

### Расчёт длины закодированной строки

```c
#include "base64.h"

int base64_encode_inline_len(int len);
```

Вычисляет длину закодированной строки на основе длины исходной.

**Параметры**\
`len` — длина исходной строки.

**Возвращаемое значение**\
Длина закодированной строки.

<br>

### Кодирование строки

```c
int base64_encode_inline(char* coded_dst, const char* plain_src, int len_plain_src);
```

Кодирует строку `plain_src` в Base64.

**Параметры**\
`coded_dst` — указатель на буфер для записи закодированной строки.\
`plain_src` — указатель на исходную строку.\
`len_plain_src` — длина исходной строки.

**Возвращаемое значение**\
Длина закодированной строки.

## Декодирование

### Расчёт длины декодированной строки

```c
int base64_decode_inline_len(const char* coded_src);
```

Вычисляет длину исходной строки на основе закодированной.

**Параметры**\
`coded_src` — указатель на закодированную строку.

**Возвращаемое значение**\
Длина декодированной строки.

<br>

### Декодирование строки

```c
int base64_decode_inline(char* plain_dst, const char* coded_src);
```

Декодирует строку `coded_src` из Base64.

**Параметры**\
`plain_dst` — указатель на буфер для записи декодированной строки.\
`coded_src` — указатель на закодированную строку.

**Возвращаемое значение**\
Длина декодированной строки.

## Примеры использования

### Кодирование данных

```c
#include "base64.h"
#include <stdlib.h>
#include <string.h>

void encode_example(httpctx_t* ctx) {
    const char* original = "Hello, World!";
    int original_len = strlen(original);

    // Вычисляем размер буфера для результата
    int encoded_len = base64_encode_inline_len(original_len);
    char* encoded = malloc(encoded_len + 1);

    if (encoded == NULL) {
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    // Кодируем
    int result_len = base64_encode_inline(encoded, original, original_len);
    encoded[result_len] = '\0';

    // encoded = "SGVsbG8sIFdvcmxkIQ=="
    ctx->response->send_data(ctx->response, encoded);

    free(encoded);
}
```

### Декодирование данных

```c
void decode_example(httpctx_t* ctx) {
    const char* encoded = "SGVsbG8sIFdvcmxkIQ==";

    // Вычисляем размер буфера для результата
    int decoded_len = base64_decode_inline_len(encoded);
    char* decoded = malloc(decoded_len + 1);

    if (decoded == NULL) {
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    // Декодируем
    int result_len = base64_decode_inline(decoded, encoded);
    decoded[result_len] = '\0';

    // decoded = "Hello, World!"
    ctx->response->send_data(ctx->response, decoded);

    free(decoded);
}
```

### Кодирование бинарных данных

```c
void encode_binary(httpctx_t* ctx) {
    // Бинарные данные (например, изображение)
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

### Использование в API

```c
void get_file_base64(httpctx_t* ctx) {
    // Получаем файл из хранилища
    file_t file = storage_file_get("local", "images/logo.png");
    if (!file.ok) {
        ctx->response->status_code = 404;
        ctx->response->send_data(ctx->response, "File not found");
        return;
    }

    // Читаем содержимое файла
    char* content = malloc(file.size);
    if (content == NULL) {
        file.close(&file);
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    // ... чтение файла в content ...

    // Кодируем в Base64
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

    // Формируем JSON-ответ
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);
    json_object_set(root, "filename", json_create_string(file.name));
    json_object_set(root, "data", json_create_string(encoded));

    ctx->response->header_add(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(doc));

    json_free(doc);
    free(encoded);
    free(content);
    file.close(&file);
}
```

### Data URL для изображений

```c
void get_image_data_url(httpctx_t* ctx) {
    const char* image_data = "..."; // бинарные данные изображения
    int image_size = 1024;          // размер данных

    int encoded_len = base64_encode_inline_len(image_size);
    // +30 для префикса "data:image/png;base64,"
    char* data_url = malloc(encoded_len + 30);

    if (data_url == NULL) {
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    // Формируем Data URL
    strcpy(data_url, "data:image/png;base64,");
    int offset = strlen(data_url);

    base64_encode_inline(data_url + offset, image_data, image_size);

    ctx->response->send_data(ctx->response, data_url);

    free(data_url);
}
```
