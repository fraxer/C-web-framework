#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"
#include "rapidjson/allocators.h"
#include "../helpers/helpers.h"
#include "../log/log.h"
#include "config.h"
#include <cstdio>

namespace config {

namespace {

    char* CONFIG_PATH = nullptr;

    rapidjson::Document content;

} // namespace

char* getPath(int argc, char* argv[]) {
    int opt = 0;

    char* path = nullptr;

    bool c_found = false;

    opterr = 0;
    optopt = 0;
    optind = 0;
    optarg = nullptr;

    while ((opt = getopt(argc, argv, "c:")) != -1) {
        switch (opt) {
            case 'c':
                path = optarg;
                c_found = true;
                break;
            default:
                return nullptr;
        }
    }

    if (!c_found) {
        return nullptr;
    }

    if (argc < 3) {
        return nullptr;
    }

    return path;
}

int savePath(const char* path) {
    if (path == nullptr) {
        return -1;
    }

    if (strlen(path) == 0) {
        return -1;
    }

    if (CONFIG_PATH != nullptr) {
        ::free(CONFIG_PATH);
    }

    CONFIG_PATH = (char*)malloc(sizeof(char) * strlen(path) + 1);

    if (CONFIG_PATH == nullptr) {
        return -1;
    }

    strcpy(CONFIG_PATH, path);

    return 0;
}

int load(const char* path) {
    if (path == nullptr) {
        return -1;
    }

    int fd = open(path, O_RDONLY);

    if (fd == -1) return -1;

    char* data = readFile(fd);

    close(fd);

    if (data == nullptr) return -1;

    int result = -1;

    if (parseData(data) == -1) goto failed;

    result = 0;

    failed:

    ::free(data);

    return result;
}

int reload() {
    if (CONFIG_PATH == nullptr) {
        return -1;
    }

    if (strlen(CONFIG_PATH) == 0) {
        return -1;
    }

    return load(CONFIG_PATH);
}

int init(int argc, char* argv[]) {
    char* configPath = getPath(argc, argv);

    if (configPath == nullptr) {
        log::print("Usage: -c <path to config file>\n", "");
        return -1;
    }

    if (load(configPath) == -1) {
        log::print("Can't load config\n", "");
        return -1;
    }

    if (savePath(configPath) == -1) {
        log::print("Can't save config path\n", "");
        return -1;
    }

    return 0;
}

int free() {

    rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator = content.GetAllocator();

    allocator.Clear();

    return 0;
}

int parseData(const char* data) {
    rapidjson::ParseResult parseSuccess = content.Parse(data);

    if (!parseSuccess) {
        return -1;
    }

    if (!content.IsObject()) {
        return -1;
    }

    return 0;
}

const rapidjson::Value* getSection(const char* section) {

    if (!content.HasMember(section)) {
        return nullptr;
    }

    return &content[section];
}

} // namespace
