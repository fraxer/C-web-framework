#include "http1.h"
#include "storage.h"

void file_create_tmpfile(__attribute__((unused))http1request_t* request, http1response_t* response) {
    file_t file = file_create_tmp("mytmp.txt");
    if (!file.ok) {
        response->data(response, "error");
        return;
    }

    file.close(&file);

    response->data(response, "done");
}

void file_get_content(__attribute__((unused))http1request_t* request, http1response_t* response) {
    file_t file = storage_file_get("local", "%s/%s", "/folder/", "/file.txt");
    if (!file.ok) {
        response->data(response, "error");
        return;
    }

    char* data = file.content(&file);

    response->data(response, data);

    free(data);
    file.close(&file);
}

void file_put_storage(__attribute__((unused))http1request_t* request, http1response_t* response) {
    file_t file = file_create_tmp("file.txt");
    if (!file.ok) {
        response->data(response, "error");
        return;
    }

    const char* str = "file content";
    file.set_content(&file, str, strlen(str));

    const char* result = "done";
    if (!storage_file_put("local", &file, "folder/%s", file.name))
        result = "can't save file";

    response->data(response, result);

    file.close(&file);
}

void file_remove_storage(__attribute__((unused))http1request_t* request, http1response_t* response) {
    const char* result = "done";
    if (!storage_file_remove("local", "folder/file.txt"))
        result = "can't remove file";

    response->data(response, result);
}

void file_upload_and_put_storage(http1request_t* request, http1response_t* response) {
    file_content_t payload_content = request->payload_filef(request, "myfile");
    if (!payload_content.ok) {
        response->data(response, "file not found");
        return;
    }

    storage_file_content_put("remote", &payload_content, "folder/%s", payload_content.filename);

    response->data(response, "done");
}

void file_duplicate_to_storage(__attribute__((unused))http1request_t* request, http1response_t* response) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "folder/%s", "nike.png");

    if (!storage_file_exist("local", path))
        storage_file_duplicate("remote", "local", path);

    response->filef(response, "local", path);
}
