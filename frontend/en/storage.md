---
outline: deep
description: File storage in C Web Framework. Local storage and S3-compatible services. Uploading, retrieving and deleting files.
---

# File Storage

The framework provides a unified API for working with files through various storages: local file system and S3-compatible services.

## Configuration

Storages are configured in the `storages` section of the `config.json` file:

### Local Storage

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

### S3-Compatible Storage

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

## Storage API

### Getting a File

```c
#include "storage.h"

file_t storage_file_get(const char* storage_name, const char* path_format, ...);
```

Retrieves a file from storage.

**Parameters**\
`storage_name` — storage name from configuration.\
`path_format` — path to the file (supports formatting like `printf`).

**Return Value**\
A `file_t` structure. The `ok` field equals 1 on success.

<br>

### Saving a File

```c
int storage_file_put(const char* storage_name, file_t* file, const char* path_format, ...);
```

Saves a file to storage.

**Parameters**\
`storage_name` — storage name.\
`file` — pointer to file structure.\
`path_format` — path to save.

**Return Value**\
Non-zero value on success.

<br>

### Saving File Content

```c
int storage_file_content_put(const char* storage_name, file_content_t* file_content, const char* path_format, ...);
```

Saves file content (e.g., from a multipart form) to storage.

**Parameters**\
`storage_name` — storage name.\
`file_content` — pointer to file content structure.\
`path_format` — path to save.

**Return Value**\
Non-zero value on success.

<br>

### Saving Data

```c
int storage_file_data_put(const char* storage_name, const char* data, const size_t data_size, const char* path_format, ...);
```

Saves arbitrary data as a file.

**Parameters**\
`storage_name` — storage name.\
`data` — pointer to data.\
`data_size` — data size.\
`path_format` — path to save.

**Return Value**\
Non-zero value on success.

<br>

### Deleting a File

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

### Checking Existence

```c
int storage_file_exist(const char* storage_name, const char* path_format, ...);
```

Checks if a file exists.

**Parameters**\
`storage_name` — storage name.\
`path_format` — path to the file.

**Return Value**\
Non-zero value if the file exists.

<br>

### Copying Between Storages

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

### File List

```c
array_t* storage_file_list(const char* storage_name, const char* path_format, ...);
```

Gets a list of files in a directory.

**Parameters**\
`storage_name` — storage name.\
`path_format` — path to the directory.

**Return Value**\
Array of strings with file names. Must be freed with `array_free()`.

## Working with Temporary Files

```c
#include "file.h"

file_t file_create_tmp(const char* filename);
```

Creates a temporary file.

**Parameters**\
`filename` — file name.

**Return Value**\
A `file_t` structure. The `ok` field equals 1 on success.

## Usage Examples

### Uploading a File from User

```c
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

### Sending a File to User

```c
void download(httpctx_t* ctx) {
    // Send file from storage
    ctx->response->send_filef(ctx->response, "local", "documents/report.pdf");
}
```

### Creating and Saving a File

```c
void create_file(httpctx_t* ctx) {
    // Create temporary file
    file_t file = file_create_tmp("data.txt");
    if (!file.ok) {
        ctx->response->send_data(ctx->response, "Failed to create temp file");
        return;
    }

    // Write content
    const char* content = "Hello, World!";
    file.set_content(&file, content, strlen(content));

    // Save to storage
    if (!storage_file_put("local", &file, "texts/%s", file.name)) {
        file.close(&file);
        ctx->response->send_data(ctx->response, "Failed to save file");
        return;
    }

    file.close(&file);
    ctx->response->send_data(ctx->response, "File created");
}
```

### Listing Files in Directory

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

### Copying from S3 to Local Storage

```c
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

## file_t Structure

```c
typedef struct file {
    int ok;                    // Operation status
    char name[NAME_MAX];       // File name
    char path[PATH_MAX];       // Full path
    size_t size;               // File size

    void(*close)(struct file*);
    int(*set_content)(struct file*, const char* data, size_t size);
    // ...
} file_t;
```
