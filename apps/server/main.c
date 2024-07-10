#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <linux/limits.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "appconfig.h"
#include "moduleloader.h"
#include "log.h"
#include "signal/signal.h"

static pthread_cond_t main_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t main_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char* argv[]) {
    int result = EXIT_FAILURE;

    if (strcmp(CMAKE_BUILD_TYPE, "Release") == 0)
        if (daemon(1, 1) < 0) goto failed;

    log_init();
    signal_init();
    if (!appconfig_init(argc, argv))
        goto failed;
    if (!module_loader_init(appconfig()))
        goto failed;

    result = EXIT_SUCCESS;

    pthread_mutex_lock(&main_mutex);
    pthread_cond_wait(&main_cond, &main_mutex);
    pthread_mutex_unlock(&main_mutex);

    failed:

    signal_before_terminate(0);

    return result;
}