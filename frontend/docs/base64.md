---
outline: deep
description: Библиотека Base64 в C Web Framework. Кодирование и декодирование данных в формат Base64.
---

# Библиотека Base64

Библиотека для кодирования и декодирования данных в формат Base64 (RFC 4648). Алфавит — стандартный `A–Z`, `a–z`, `0–9`, `+`, `/`, дополнение символом `=`.

::: tip Нулевой байт
Все функции кодирования и декодирования **завершают результат нулевым байтом**. Функции семейства `*_len` возвращают размер буфера **с учётом** нулевого байта — то есть `malloc(base64_*_len(...))` выделяет ровно столько памяти, сколько нужно. Сами функции `encode`/`decode` возвращают длину полезных данных **без** учёта нулевого байта.
:::

## Кодирование

### Расчёт размера буфера

```c
#include "base64.h"

int base64_encode_len(const int len);
```

Вычисляет размер буфера под результат кодирования строки длиной `len`, включая нулевой байт.

**Параметры**\
`len` — длина исходных данных.

**Возвращаемое значение**\
Размер буфера в байтах (с учётом `\0`).

<br>

### Кодирование

```c
int base64_encode(char* encoded, const char* string, const int string_len);
```

Кодирует `string_len` байт из `string` в Base64 и дописывает нулевой байт.

**Параметры**\
`encoded` — буфер для записи результата (размер не менее `base64_encode_len(string_len)`).\
`string` — исходные данные.\
`string_len` — длина исходных данных.

**Возвращаемое значение**\
Длина закодированной строки **без** учёта нулевого байта.

<br>

### Кодирование с переносом строк

```c
int base64_encode_nl_len(const int len, int wrap);
int base64_encode_nl(char* encoded, const char* string, const int string_len, const int wrap);
```

Кодирует данные, вставляя `\r\n` каждые `wrap` символов результата — формат, принятый в PEM и MIME (RFC 2045, типичная ширина — 76 символов). При `wrap == 0` ведёт себя как обычное кодирование.

**Параметры**\
`len` — длина исходных данных (для `*_len`).\
`encoded` — буфер для записи результата.\
`string` — исходные данные.\
`string_len` — длина исходных данных.\
`wrap` — ширина строки в символах; `0` отключает перенос.

**Возвращаемое значение**\
`base64_encode_nl_len` — размер буфера с учётом `\0` и символов переноса.\
`base64_encode_nl` — длина закодированных данных **без** учёта нулевого байта.

## Декодирование

### Расчёт размера буфера

```c
int base64_decode_len(const char* bufcoded);
```

Вычисляет верхнюю оценку размера декодированных данных по закодированной строке, включая нулевой байт. Символы `\r` и `\n` в `bufcoded` пропускаются, поэтому строка может содержать переносы.

**Параметры**\
`bufcoded` — закодированная строка (с `\0`).

**Возвращаемое значение**\
Размер буфера в байтах (с учётом `\0`).

<br>

### Декодирование

```c
int base64_decode(char* bufplain, const char* bufcoded);
```

Декодирует `bufcoded` из Base64 и дописывает нулевой байт. Символы `\r` и `\n` в входной строке игнорируются.

**Параметры**\
`bufplain` — буфер для записи результата (размер не менее `base64_decode_len(bufcoded)`).\
`bufcoded` — закодированная строка (с `\0`).

**Возвращаемое значение**\
Длина декодированных данных **без** учёта нулевого байта.

## Примеры использования

### Кодирование данных

```c
#include "http.h"
#include "base64.h"
#include <stdlib.h>
#include <string.h>

void encode_example(httpctx_t* ctx) {
    const char* original = "Hello, World!";
    int original_len = strlen(original);

    // base64_encode_len уже учитывает нулевой байт
    char* encoded = malloc(base64_encode_len(original_len));
    if (encoded == NULL) {
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    // Кодируем — результат завершается нулевым байтом
    base64_encode(encoded, original, original_len);

    // encoded = "SGVsbG8sIFdvcmxkIQ=="
    ctx->response->send_data(ctx->response, encoded);

    free(encoded);
}
```

### Декодирование данных

```c
void decode_example(httpctx_t* ctx) {
    const char* encoded = "SGVsbG8sIFdvcmxkIQ==";

    // base64_decode_len уже учитывает нулевой байт
    char* decoded = malloc(base64_decode_len(encoded));
    if (decoded == NULL) {
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    // Декодируем — результат завершается нулевым байтом
    base64_decode(decoded, encoded);

    // decoded = "Hello, World!"
    ctx->response->send_data(ctx->response, decoded);

    free(decoded);
}
```

### Кодирование бинарных данных

```c
void encode_binary(httpctx_t* ctx) {
    // Бинарные данные (например, сигнатура PNG)
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

### Кодирование с переносом строк (PEM/MIME)

```c
void encode_pem(httpctx_t* ctx) {
    const char* original = "Very long payload ...";
    int original_len = strlen(original);

    // 76 — стандартная ширина строки для PEM/MIME
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

### Использование в API

```c
#include "http.h"
#include "base64.h"
#include "storage.h"
#include <stdlib.h>

void get_file_base64(httpctx_t* ctx) {
    // Получаем файл из хранилища
    file_t file = storage_file_get("local", "images/logo.png");
    if (!file.ok) {
        ctx->response->status_code = 404;
        ctx->response->send_data(ctx->response, "File not found");
        return;
    }

    // Читаем содержимое файла
    char* content = file.content(&file);
    if (content == NULL) {
        file.close(&file);
        ctx->response->send_data(ctx->response, "Read error");
        return;
    }

    // Кодируем в Base64
    char* encoded = malloc(base64_encode_len(file.size));
    if (encoded == NULL) {
        free(content);
        file.close(&file);
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    base64_encode(encoded, content, file.size);

    // Формируем JSON-ответ
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

### Data URL для изображений

```c
void get_image_data_url(httpctx_t* ctx) {
    const char* image_data = "..."; // бинарные данные изображения
    int image_size = 1024;          // размер данных

    const char* prefix = "data:image/png;base64,";
    // base64_encode_len уже учитывает нулевой байт; +strlen(prefix) под префикс
    char* data_url = malloc(base64_encode_len(image_size) + strlen(prefix));
    if (data_url == NULL) {
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    // Записываем префикс, затем кодируем данные сразу за ним
    strcpy(data_url, prefix);
    base64_encode(data_url + strlen(prefix), image_data, image_size);

    ctx->response->send_data(ctx->response, data_url);

    free(data_url);
}
```
