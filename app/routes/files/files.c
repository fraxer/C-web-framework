#include "http1.h"
#include "storage.h"

void file_create_tmpfile(httpctx_t* ctx) {
    file_t file = file_create_tmp("mytmp.txt");
    if (!file.ok) {
        ctx->response->data(ctx->response, "error");
        return;
    }

    file.close(&file);

    ctx->response->data(ctx->response, "done");
}

void file_get_content(httpctx_t* ctx) {
    file_t file = storage_file_get("local", "%s/%s", "/folder/", "/file.txt");
    if (!file.ok) {
        ctx->response->data(ctx->response, "error");
        return;
    }

    char* data = file.content(&file);

    ctx->response->data(ctx->response, data);

    free(data);
    file.close(&file);
}

void file_put_storage(httpctx_t* ctx) {
    file_t file = file_create_tmp("file.txt");
    if (!file.ok) {
        ctx->response->data(ctx->response, "error");
        return;
    }

    const char* str = "file content";
    file.set_content(&file, str, strlen(str));

    const char* result = "done";
    if (!storage_file_put("local", &file, "folder/%s", file.name))
        result = "can't save file";

    ctx->response->data(ctx->response, result);

    file.close(&file);
}

void file_remove_storage(httpctx_t* ctx) {
    const char* result = "done";
    if (!storage_file_remove("local", "folder/file.txt"))
        result = "can't remove file";

    ctx->response->data(ctx->response, result);
}

void file_upload_and_put_storage(httpctx_t* ctx) {
    file_content_t payload_content = ctx->request->payload_filef(ctx->request, "myfile");
    if (!payload_content.ok) {
        ctx->response->data(ctx->response, "file not found");
        return;
    }

    storage_file_content_put("remote", &payload_content, "folder/%s", payload_content.filename);

    ctx->response->data(ctx->response, "done");
}

void file_duplicate_to_storage(httpctx_t* ctx) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "folder/%s", "nike.png");

    if (!storage_file_exist("local", path))
        storage_file_duplicate("remote", "local", path);

    ctx->response->filef(ctx->response, "local", path);
}