#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "moduleloader/moduleloader.h"
#include "log/log.h"
#include "signal/signal.h"

static pthread_cond_t main_cond = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t main_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char* argv[]) {

    int result = EXIT_FAILURE;

    if (daemon(1, 1) < 0) goto failed;

    log_init();

    if (signal_init() == -1) goto failed;

    if (module_loader_init(argc, argv) == -1) goto failed;

    result = EXIT_SUCCESS;

    pthread_mutex_lock(&main_mutex);

    pthread_cond_wait(&main_cond, &main_mutex);

    pthread_mutex_unlock(&main_mutex);

    failed:

    signal_before_terminate(0);

    return result;
}