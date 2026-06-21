---
outline: deep
description: File storage in C Web Framework. Local storage and S3-compatible services. Uploading, retrieving and deleting files.
---

# File Storage

The framework provides a unified API for working with files through various storages: a local file system and S3-compatible services. All functions are available through a single interface regardless of the storage type.

## Configuration

Storages are configured in the `storages` section of the `config.json` file. It is a named map where the key is a unique storage name used to reference it in code:

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

### Local storage (`filesystem`)

- `type` — `filesystem`
- `root` — absolute path to the storage root directory (trailing `/` characters are trimmed)

### S3-compatible storage (`s3`)

- `type` — `s3`
- `access_id` — access key
- `access_secret` — secret key
- `protocol` — `http` or `https`
- `host` — storage host (e.g. `s3.amazonaws.com`)
- `port` — port (an empty string means the default port)
- `bucket` — bucket name
- `region` — region (e.g. `us-east-1`)

::: tip All S3 fields are required
For the `s3` driver, every field listed above is required — the storage will not load if any of them is missing.
:::

### Temporary files

To work with temporary files, specify a directory in the `main` section:

```json
{
    "main": {
        "tmp": "/tmp"
    }
}
```

The value is available in code as `env()->main.tmp` and is used when creating temporary files via `file_create_tmp()`.

## Storage API

```c
#include "storage.h"
```

### Getting a file

```c
file_t storage_file_get(const char* storage_name, const char* path_format, ...);
```

Retrieves a file from storage and opens it for reading. For the `filesystem` driver, a path containing `*` is treated as a glob pattern — the first matching file is opened.

**Parameters**\
`storage_name` — storage name from configuration.\
`path_format` — path to the file relative to the storage root (supports formatting like `printf`).

**Return Value**\
A `file_t` structure. The `ok` field equals 1 on success.

<br>

### Saving a file

```c
int storage_file_put(const char* storage_name, file_t* file, const char* path_format, ...);
```

Saves an open file (a `file_t` structure) to storage.

**Parameters**\
`storage_name` — storage name.\
`file` — pointer to the file structure.\
`path_format` — path to save.

**Return Value**\
Non-zero value on success.

<br>

### Saving file content

```c
int storage_file_content_put(const char* storage_name, file_content_t* file_content, const char* path_format, ...);
```

Saves file content (e.g. data from a multipart form) to storage.

**Parameters**\
`storage_name` — storage name.\
`file_content` — pointer to the file content structure.\
`path_format` — path to save.

**Return Value**\
Non-zero value on success.

<br>

### Saving data

```c
int storage_file_data_put(const char* storage_name, const char* data, const size_t data_size, const char* path_format, ...);
```

Saves arbitrary data as a file.

**Parameters**\
`storage_name` — storage name.\
`data` — pointer to the data.\
`data_size` — data size in bytes.\
`path_format` — path to save.

**Return Value**\
Non-zero value on success.

<br>

### Deleting a file

```c
int storage_file_remove(const char* storage_name, const char* path_format, ...);
```

Deletes a file from storage.

**Parameters**\
`storage_name` — storage name.\
`path_format` — path to the file.

**Return Value**\
Non-zero value on success.

<br>

### Checking existence

```c
int storage_file_exist(const char* storage_name, const char* path_format, ...);
```

Checks whether a file exists in storage.

**Parameters**\
`storage_name` — storage name.\
`path_format` — path to the file.

**Return Value**\
Non-zero value if the file exists.

<br>

### Copying between storages

```c
int storage_file_duplicate(const char* from_storage_name, const char* to_storage_name, const char* path_format, ...);
```

Copies a file from one storage to another.

**Parameters**\
`from_storage_name` — source storage name.\
`to_storage_name` — target storage name.\
`path_format` — path to the file.

**Return Value**\
Non-zero value on success.

<br>

### File list

```c
array_t* storage_file_list(const char* storage_name, const char* path_format, ...);
```

Gets a list of files in a storage directory.

**Parameters**\
`storage_name` — storage name.\
`path_format` — path to the directory.

**Return Value**\
Array of strings with file names. Must be freed with `array_free()`. Returns `NULL` on error.

## Working with files

```c
#include "file.h"
```

### Creating a temporary file

```c
file_t file_create_tmp(const char* filename, const char* tmp_path);
```

Creates a temporary file in the given directory. The file is automatically deleted from disk when `close()` is called (the `tmp` field is set to 1).

**Parameters**\
`filename` — file name (used only for the `name` field).\
`tmp_path` — directory where the file is created (typically `env()->main.tmp`).

**Return Value**\
A `file_t` structure. The `ok` field equals 1 on success.

<br>

### Opening a file

```c
file_t file_open(const char* path, const int flags);
```

Opens an existing file or creates a new one with the given flags (`O_RDONLY`, `O_WRONLY`, `O_RDWR`, `O_CREAT`, etc.).

**Parameters**\
`path` — full path to the file.\
`flags` — open flags.

**Return Value**\
A `file_t` structure. The `ok` field equals 1 on success.

<br>

### Empty file structure

```c
file_t file_alloc();
```

Creates an empty `file_t` structure with initialized methods (`ok=0`, `fd=-1`). Useful as a safe return value on errors.

**Return Value**\
A `file_t` structure with `ok=0`.

<br>

### Creating file content

```c
file_content_t file_content_create(const int fd, const char* filename, const off_t offset, const size_t size);
```

Creates a `file_content_t` structure for working with a portion of a file descriptor's data (e.g. when processing a multipart form).

**Parameters**\
`fd` — source file descriptor.\
`filename` — name for the file to be created.\
`offset` — offset in the source where data starts.\
`size` — data size in bytes.

**Return Value**\
A `file_content_t` structure with initialized methods.

## Usage examples

### Uploading a file from a user

```c
#include "http.h"
#include "storage.h"

void upload(httpctx_t* ctx) {
    // Get file from multipart form
    file_content_t payload = ctx->request->get_payload_filef(ctx->request, "myfile");
    if (!payload.ok) {
        ctx->response->send_data(ctx->response, "File not found in request");
        return;
    }

    // Save to storage
    if (!storage_file_content_put("local", &payload, "uploads/%s", payload.filename)) {
        ctx->response->send_data(ctx->response, "Failed to save file");
        return;
    }

    ctx->response->send_data(ctx->response, "File uploaded successfully");
}
```

### Sending a file to a user

```c
#include "http.h"

void download(httpctx_t* ctx) {
    // Send file from storage (Content-Type is detected by extension)
    ctx->response->send_filef(ctx->response, "local", "documents/report.pdf");
}
```

### Creating and saving a file

```c
#include "http.h"
#include "storage.h"
#include "appconfig.h"

void create_file(httpctx_t* ctx) {
    // Create temporary file
    file_t file = file_create_tmp("data.txt", env()->main.tmp);
    if (!file.ok) {
        ctx->response->send_data(ctx->response, "Failed to create temp file");
        return;
    }

    // Write content
    const char* content = "Hello, World!";
    file.set_content(&file, content, strlen(content));

    // Save to storage
    const char* result = "File created";
    if (!storage_file_put("local", &file, "texts/%s", file.name))
        result = "Failed to save file";

    file.close(&file);
    ctx->response->send_data(ctx->response, result);
}
```

### Saving arbitrary data

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

### Listing files in a directory

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

### Copying from S3 to local storage

```c
#include "http.h"
#include "storage.h"

void cache_from_s3(httpctx_t* ctx) {
    const char* path = "images/photo.jpg";

    // Check if exists in local cache
    if (!storage_file_exist("local", path)) {
        // Copy from S3
        storage_file_duplicate("remote", "local", path);
    }

    // Send from local storage
    ctx->response->send_filef(ctx->response, "local", path);
}
```

## Data structures

### file_t

A file handle with an object-oriented interface.

```c
typedef struct file {
    size_t size;               // File size in bytes
    time_t mtime;              // Last modification time

    int(*set_name)(struct file*, const char* name);
    char*(*content)(struct file*);                                       // reads content (free with free())
    int(*set_content)(struct file*, const char* data, const size_t size); // overwrites content
    int(*append_content)(struct file*, const char* data, const size_t size);
    int(*close)(struct file*);                                           // closes, deletes temporary file
    int(*truncate)(struct file*, const off_t offset);

    int fd;                    // File descriptor (-1 if not open)
    unsigned tmp;              // 1 = temporary file, deleted on close()
    unsigned ok;               // 1 = file opened/created successfully
    char name[NAME_MAX];       // File name (without path)
} file_t;
```

::: warning Releasing resources
The `content()` method returns a buffer allocated with `malloc()` — it must be freed with `free()`. Always call `close()` when you are done working with a file.
:::

### file_content_t

File content from an external source (e.g. a portion of a multipart form).

```c
typedef struct file_content {
    off_t offset;              // Offset in the source descriptor where data starts
    size_t size;               // Data size in bytes

    int(*set_filename)(struct file_content*, const char* filename);
    file_t(*make_file)(struct file_content*, const char* filepath, const char* filename);
    file_t(*make_tmpfile)(struct file_content*, const char* tmp_path);
    char*(*content)(struct file_content*);   // reads data into memory (free with free())

    int fd;                    // Source file descriptor
    int ok;                    // 1 = structure is valid
    char filename[NAME_MAX];   // Name for the file to be created
} file_content_t;
```
