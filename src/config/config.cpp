#include "config.h"

static char* CONFIG_PATH = nullptr;

char* getConfigPath(int argc, char* argv[]) {
    int opt = 0;

    char* path;

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
                throw "Usage: -c <path to config file>";
        }
    }

    if (!c_found) {
        throw "Expected -c option";
    }

    if (argc < 3) {
        throw "Expected argument after options";
    }

    return path;
}

void saveConfigPath(const char* path) {
    if (path == nullptr) {
        throw "Config path is nullptr";
    }

    if (strlen(path) == 0) {
        throw "Config path is empty";
    }

    if (CONFIG_PATH != nullptr) {
        free(CONFIG_PATH);
    }

    CONFIG_PATH = (char*)malloc(sizeof(char) * strlen(path) + 1);

    if (CONFIG_PATH == nullptr) {
        throw "Error allocate memory for config path";
    }

    strcpy(CONFIG_PATH, path);
}

void loadConfig(const char* path) {
    int fd = open(path, O_RDONLY);

    if (fd == -1) throw "Config file not found";

    char* data = readFile(fd);

    // proccessing
    throw "proccessing";

    close(fd);
}

void reloadConfig() {
    loadConfig(CONFIG_PATH);
}