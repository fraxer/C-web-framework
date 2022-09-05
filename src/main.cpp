#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>
#include "config/config.h"

char* readFile(int fd);

long int filesize(int fd) {

    struct stat stat_buf;

    int r = fstat(fd, &stat_buf);

    return r == 0 ? stat_buf.st_size : -1;
}

char* readFile(int fd) {

    long int size = filesize(fd);

    char* buf = (char*)malloc(size + 1);

    if (buf == nullptr) return nullptr;

    long int bytesRead = read(fd, buf, size);

    if (bytesRead == -1) return nullptr;

    return buf;
}

int main(int argc, char* argv[]) {

    // if (daemon(1, 1) < 0) return EXIT_FAILURE;

    openlog(NULL, LOG_CONS | LOG_NDELAY, LOG_USER);

    try {
        loadConfig(getPathConfigFile(argc, argv));

    } catch (const char* message) {
        printf("%s\n", message);

        syslog(LOG_ERR, "%s\n", message);

        closelog();

        exit(EXIT_FAILURE);
    }

    

    // initSignalHandlers();

    // initRouteHandlers();

    // initDBConnection();

    // initOpenSSL();

    // initServerContext();

    // initMimeTypes();

    // initThreads();

    closelog();

    return EXIT_SUCCESS;
}