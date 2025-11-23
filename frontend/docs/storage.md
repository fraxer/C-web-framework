---
outline: deep
description: Файловое хранилище в C Web Framework. Локальное хранилище и S3-совместимые сервисы. Загрузка, получение и удаление файлов.
---

# Файловое хранилище

Фреймворк предоставляет унифицированный API для работы с файлами через различные хранилища: локальную файловую систему и S3-совместимые сервисы.

## Конфигурация

Хранилища настраиваются в секции `storages` файла `config.json`:

### Локальное хранилище

```json
{
    "storages": {
        "local": {
            "type": "filesystem",
            "root": "/var/www/storage"
        }
    }
}
```

### S3-совместимое хранилище

```json
{
    "storages": {
        "remote": {
            "type": "s3",
            "access_id": "your-access-key",
            "access_secret": "your-secret-key",
            "protocol": "https",
            "host": "s3.amazonaws.com",
            "port": "",
            "bucket": "my-bucket",
            "region": "us-east-1"
        }
    }
}
```

## API хранилища

### Получение файла

```c
#include "storage.h"

file_t storage_file_get(const char* storage_name, const char* path_format, ...);
```

Получает файл из хранилища.

**Параметры**\
`storage_name` — имя хранилища из конфигурации.\
`path_format` — путь к файлу (поддерживает форматирование как `printf`).

**Возвращаемое значение**\
Структура `file_t`. Поле `ok` равно 1 при успехе.

<br>

### Сохранение файла

```c
int storage_file_put(const char* storage_name, file_t* file, const char* path_format, ...);
```

Сохраняет файл в хранилище.

**Параметры**\
`storage_name` — имя хранилища.\
`file` — указатель на структуру файла.\
`path_format` — путь для сохранения.

**Возвращаемое значение**\
Ненулевое значение при успехе.

<br>

### Сохранение содержимого файла

```c
int storage_file_content_put(const char* storage_name, file_content_t* file_content, const char* path_format, ...);
```

Сохраняет содержимое файла (например, из multipart-формы) в хранилище.

**Параметры**\
`storage_name` — имя хранилища.\
`file_content` — указатель на структуру содержимого файла.\
`path_format` — путь для сохранения.

**Возвращаемое значение**\
Ненулевое значение при успехе.

<br>

### Сохранение данных

```c
int storage_file_data_put(const char* storage_name, const char* data, const size_t data_size, const char* path_format, ...);
```

Сохраняет произвольные данные как файл.

**Параметры**\
`storage_name` — имя хранилища.\
`data` — указатель на данные.\
`data_size` — размер данных.\
`path_format` — путь для сохранения.

**Возвращаемое значение**\
Ненулевое значение при успехе.

<br>

### Удаление файла

```c
int storage_file_remove(const char* storage_name, const char* path_format, ...);
```

Удаляет файл из хранилища.

**Параметры**\
`storage_name` — имя хранилища.\
`path_format` — путь к файлу.

**Возвращаемое значение**\
Ненулевое значение при успехе.

<br>

### Проверка существования

```c
int storage_file_exist(const char* storage_name, const char* path_format, ...);
```

Проверяет существование файла.

**Параметры**\
`storage_name` — имя хранилища.\
`path_format` — путь к файлу.

**Возвращаемое значение**\
Ненулевое значение, если файл существует.

<br>

### Копирование между хранилищами

```c
int storage_file_duplicate(const char* from_storage_name, const char* to_storage_name, const char* path_format, ...);
```

Копирует файл из одного хранилища в другое.

**Параметры**\
`from_storage_name` — имя исходного хранилища.\
`to_storage_name` — имя целевого хранилища.\
`path_format` — путь к файлу.

**Возвращаемое значение**\
Ненулевое значение при успехе.

<br>

### Список файлов

```c
array_t* storage_file_list(const char* storage_name, const char* path_format, ...);
```

Получает список файлов в директории.

**Параметры**\
`storage_name` — имя хранилища.\
`path_format` — путь к директории.

**Возвращаемое значение**\
Массив строк с именами файлов. Необходимо освободить функцией `array_free()`.

## Работа с временными файлами

```c
#include "file.h"

file_t file_create_tmp(const char* filename);
```

Создает временный файл.

**Параметры**\
`filename` — имя файла.

**Возвращаемое значение**\
Структура `file_t`. Поле `ok` равно 1 при успехе.

## Примеры использования

### Загрузка файла от пользователя

```c
void upload(httpctx_t* ctx) {
    // Получаем файл из multipart-формы
    file_content_t payload = ctx->request->get_payload_filef(ctx->request, "myfile");
    if (!payload.ok) {
        ctx->response->send_data(ctx->response, "File not found in request");
        return;
    }

    // Сохраняем в хранилище
    if (!storage_file_content_put("local", &payload, "uploads/%s", payload.filename)) {
        ctx->response->send_data(ctx->response, "Failed to save file");
        return;
    }

    ctx->response->send_data(ctx->response, "File uploaded successfully");
}
```

### Отправка файла пользователю

```c
void download(httpctx_t* ctx) {
    // Отправляем файл из хранилища
    ctx->response->send_filef(ctx->response, "local", "documents/report.pdf");
}
```

### Создание и сохранение файла

```c
void create_file(httpctx_t* ctx) {
    // Создаем временный файл
    file_t file = file_create_tmp("data.txt");
    if (!file.ok) {
        ctx->response->send_data(ctx->response, "Failed to create temp file");
        return;
    }

    // Записываем содержимое
    const char* content = "Hello, World!";
    file.set_content(&file, content, strlen(content));

    // Сохраняем в хранилище
    if (!storage_file_put("local", &file, "texts/%s", file.name)) {
        file.close(&file);
        ctx->response->send_data(ctx->response, "Failed to save file");
        return;
    }

    file.close(&file);
    ctx->response->send_data(ctx->response, "File created");
}
```

### Список файлов в директории

```c
void list_files(httpctx_t* ctx) {
    array_t* files = storage_file_list("local", "uploads/");
    if (files == NULL) {
        ctx->response->send_data(ctx->response, "Failed to list files");
        return;
    }

    str_t* result = str_create_empty(1024);
    for (size_t i = 0; i < array_size(files); i++) {
        const char* filename = array_get(files, i);
        str_append(result, filename, strlen(filename));
        str_appendc(result, '\n');
    }

    ctx->response->send_data(ctx->response, str_get(result));

    str_free(result);
    array_free(files);
}
```

### Копирование из S3 в локальное хранилище

```c
void cache_from_s3(httpctx_t* ctx) {
    const char* path = "images/photo.jpg";

    // Проверяем наличие в локальном кеше
    if (!storage_file_exist("local", path)) {
        // Копируем из S3
        storage_file_duplicate("remote", "local", path);
    }

    // Отправляем из локального хранилища
    ctx->response->send_filef(ctx->response, "local", path);
}
```

## Структура file_t

```c
typedef struct file {
    int ok;                    // Статус операции
    char name[NAME_MAX];       // Имя файла
    char path[PATH_MAX];       // Полный путь
    size_t size;               // Размер файла

    void(*close)(struct file*);
    int(*set_content)(struct file*, const char* data, size_t size);
    // ...
} file_t;
```
