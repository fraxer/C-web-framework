#include <stdlib.h>
#include <unistd.h>
#include "moduleloader/moduleloader.h"
#include "log/log.h"
#include "signal/signal.h"

int main(int argc, char* argv[]) {

    int result = EXIT_FAILURE;

    if (daemon(1, 1) < 0) goto failed;

    log_init();

    if (signal_init() == -1) goto failed;

    if (module_loader_init(argc, argv) == -1) goto failed;

    result = EXIT_SUCCESS;

    failed:

    signal_before_terminate(0);

    return result;
}