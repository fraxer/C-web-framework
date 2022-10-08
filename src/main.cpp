#include <stdlib.h>
#include <unistd.h>
#include "moduleLoader/moduleLoader.h"
#include "log/log.h"
#include "signal/signal.h"

int main(int argc, char* argv[]) {

    int result = EXIT_FAILURE;

    if (daemon(1, 1) < 0) goto failed;

    if (log::init() == -1) goto failed;

    if (signl::init() == -1) goto failed;

    if (moduleLoader::init(argc, argv) == -1) goto failed;

    result = EXIT_SUCCESS;

    failed:

    signl::beforeTerminate(0);

    return result;
}