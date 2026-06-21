---
outline: deep
description: Файловое хранилище в C Web Framework. Локальное хранилище и S3-совместимые сервисы. Загрузка, получение и удаление файлов.
---

# Файловое хранилище

Фреймворк предоставляет унифицированный API для работы с файлами через различные хранилища: локальную файловую систему и S3-совместимые сервисы. Все функции доступны через единый интерфейс независимо от типа хранилища.

## Конфигурация

Хранилища настраиваются в секции `storages` файла `config.json`. Это именованная карта, где ключ — уникальное имя хранилища, используемое для обращения к нему в коде:

```json
{
    "storages": {
        "local": {
            "type": "filesystem",
            "root": "/var/www/storage"
        },
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

### Локальное хранилище (`filesystem`)

- `type` — `filesystem`
- `root` — абсолютный путь к корневой директории хранилища (завершающие `/` обрезаются)

### S3-совместимое хранилище (`s3`)

- `type` — `s3`
- `access_id` — ключ доступа (access key)
- `access_secret` — секретный ключ (secret key)
- `protocol` — `http` или `https`
- `host` — хранилища (например, `s3.amazonaws.com`)
- `port` — порт (пустая строка означает порт по умолчанию)
- `bucket` — имя бакета
- `region` — регион (например, `us-east-1`)

::: tip Все поля S3 обязательны
Для драйвера `s3` все перечисленные поля являются обязательными — хранилище не загрузится, если хотя бы одно из них пропущено.
:::

### Временные файлы

Для работы с временными файлами укажите директорию в секции `main`:

```json
{
    "main": {
        "tmp": "/tmp"
    }
}
```

Значение доступно в коде как `env()->main.tmp` и используется при создании временных файлов через `file_create_tmp()`.

## API хранилища

```c
#include "storage.h"
```

### Получение файла

```c
file_t storage_file_get(const char* storage_name, const char* path_format, ...);
```

Получает файл из хранилища и открывает его для чтения. Для драйвера `filesystem` путь, содержащий `*`, обрабатывается как glob-шаблон — будет открыт первый совпавший файл.

**Параметры**\
`storage_name` — имя хранилища из конфигурации.\
`path_format` — путь к файлу относительно корня хранилища (поддерживает форматирование как `printf`).

**Возвращаемое значение**\
Структура `file_t`. Поле `ok` равно 1 при успехе.

<br>

### Сохранение файла

```c
int storage_file_put(const char* storage_name, file_t* file, const char* path_format, ...);
```

Сохраняет открытый файл (структуру `file_t`) в хранилище.

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

Сохраняет содержимое файла (например, данные из multipart-формы) в хранилище.

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
`data_size` — размер данных в байтах.\
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

Проверяет существование файла в хранилище.

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

Получает список файлов в директории хранилища.

**Параметры**\
`storage_name` — имя хранилища.\
`path_format` — путь к директории.

**Возвращаемое значение**\
Массив строк с именами файлов. Необходимо освободить функцией `array_free()`. Возвращает `NULL` при ошибке.

## Работа с файлами

```c
#include "file.h"
```

### Создание временного файла

```c
file_t file_create_tmp(const char* filename, const char* tmp_path);
```

Создает временный файл в указанной директории. Файл будет автоматически удалён с диска при вызове `close()` (поле `tmp` равно 1).

**Параметры**\
`filename` — имя файла (используется только для поля `name`).\
`tmp_path` — директория для создания файла (обычно `env()->main.tmp`).

**Возвращаемое значение**\
Структура `file_t`. Поле `ok` равно 1 при успехе.

<br>

### Открытие файла

```c
file_t file_open(const char* path, const int flags);
```

Открывает существующий файл или создаёт новый с заданными флагами (`O_RDONLY`, `O_WRONLY`, `O_RDWR`, `O_CREAT` и т.д.).

**Параметры**\
`path` — полный путь к файлу.\
`flags` — флаги открытия.

**Возвращаемое значение**\
Структура `file_t`. Поле `ok` равно 1 при успехе.

<br>

### Пустая структура файла

```c
file_t file_alloc();
```

Создаёт пустую структуру `file_t` с инициализированными методами (`ok=0`, `fd=-1`). Удобна как безопасное возвращаемое значение при ошибках.

**Возвращаемое значение**\
Структура `file_t` с `ok=0`.

<br>

### Создание содержимого файла

```c
file_content_t file_content_create(const int fd, const char* filename, const off_t offset, const size_t size);
```

Создаёт структуру `file_content_t` для работы с частью данных файлового дескриптора (например, при обработке multipart-формы).

**Параметры**\
`fd` — исходный файловый дескриптор.\
`filename` — имя для создаваемого файла.\
`offset` — смещение начала данных в исходном дескрипторе.\
`size` — размер данных в байтах.

**Возвращаемое значение**\
Структура `file_content_t` с инициализированными методами.

## Примеры использования

### Загрузка файла от пользователя

```c
#include "http.h"
#include "storage.h"

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
#include "http.h"

void download(httpctx_t* ctx) {
    // Отправляем файл из хранилища (Content-Type определяется по расширению)
    ctx->response->send_filef(ctx->response, "local", "documents/report.pdf");
}
```

### Создание и сохранение файла

```c
#include "http.h"
#include "storage.h"
#include "appconfig.h"

void create_file(httpctx_t* ctx) {
    // Создаем временный файл
    file_t file = file_create_tmp("data.txt", env()->main.tmp);
    if (!file.ok) {
        ctx->response->send_data(ctx->response, "Failed to create temp file");
        return;
    }

    // Записываем содержимое
    const char* content = "Hello, World!";
    file.set_content(&file, content, strlen(content));

    // Сохраняем в хранилище
    const char* result = "File created";
    if (!storage_file_put("local", &file, "texts/%s", file.name))
        result = "Failed to save file";

    file.close(&file);
    ctx->response->send_data(ctx->response, result);
}
```

### Сохранение произвольных данных

```c
#include "http.h"
#include "storage.h"

void save_data(httpctx_t* ctx) {
    const char* data = "{\"status\":\"ok\"}";

    if (!storage_file_data_put("local", data, strlen(data), "cache/status.json")) {
        ctx->response->send_data(ctx->response, "Failed to save data");
        return;
    }

    ctx->response->send_data(ctx->response, "Data saved");
}
```

### Список файлов в директории

```c
#include "http.h"
#include "storage.h"
#include "str.h"
#include "array.h"

void list_files(httpctx_t* ctx) {
    array_t* files = storage_file_list("local", "uploads/");
    if (files == NULL) {
        ctx->response->send_data(ctx->response, "Failed to list files");
        return;
    }

    str_t* result = str_create_empty(256);
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
#include "http.h"
#include "storage.h"

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

## Структуры данных

### file_t

Дескриптор файла с объектно-ориентированным интерфейсом.

```c
typedef struct file {
    size_t size;               // Размер файла в байтах
    time_t mtime;              // Время последней модификации

    int(*set_name)(struct file*, const char* name);
    char*(*content)(struct file*);                                       // читает содержимое (освободить через free())
    int(*set_content)(struct file*, const char* data, const size_t size); // перезаписывает содержимое
    int(*append_content)(struct file*, const char* data, const size_t size);
    int(*close)(struct file*);                                           // закрывает, удаляет временный файл
    int(*truncate)(struct file*, const off_t offset);

    int fd;                    // Файловый дескриптор (-1, если не открыт)
    unsigned tmp;              // 1 = временный файл, удаляется при close()
    unsigned ok;               // 1 = файл успешно открыт/создан
    char name[NAME_MAX];       // Имя файла (без пути)
} file_t;
```

::: warning Освобождение ресурсов
Метод `content()` возвращает буфер, выделенный через `malloc()`, — его необходимо освободить функцией `free()`. После завершения работы с файлом всегда вызывайте `close()`.
:::

### file_content_t

Содержимое файла из внешнего источника (например, часть multipart-формы).

```c
typedef struct file_content {
    off_t offset;              // Смещение начала данных в исходном дескрипторе
    size_t size;               // Размер данных в байтах

    int(*set_filename)(struct file_content*, const char* filename);
    file_t(*make_file)(struct file_content*, const char* filepath, const char* filename);
    file_t(*make_tmpfile)(struct file_content*, const char* tmp_path);
    char*(*content)(struct file_content*);   // читает данные в память (освободить через free())

    int fd;                    // Исходный файловый дескриптор
    int ok;                    // 1 = структура валидна
    char filename[NAME_MAX];   // Имя для создаваемого файла
} file_content_t;
```
