#include "config.h"

char* getPathConfigFile(int argc, char* argv[]) {

    int opt = 0;
    opterr = 0;
    optopt = 0;
    optind = 0;
    optarg = nullptr;

    char* path;

    bool c_found = false;

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

void loadConfig(const char* path) {

    int fd = open(path, O_RDONLY);

    if (fd == -1) throw "Config file not found";

    // char* data = readFile(fd);

    // ...

    close(fd);
}