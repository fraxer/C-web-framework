#include <cstdio>
#include <stdlib.h>
#include <syslog.h>
#include "config/config.h"

int main(int argc, char* argv[]) {

    // if (daemon(1, 1) < 0) return EXIT_FAILURE;

    openlog(NULL, LOG_CONS | LOG_NDELAY, LOG_USER);

    try {
        char* configPath = getConfigPath(argc, argv);

        saveConfigPath(configPath);

        loadConfig(configPath);

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