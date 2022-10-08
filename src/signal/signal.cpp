#include <stdlib.h>
#include <execinfo.h>
#include <cstdio>
#include <signal.h>
#include "signal.h"
// #include "rapidjson/document.h"
#include "../log/log.h"
#include "../moduleLoader/moduleLoader.h"

namespace signl {

void flushStreams() {
    fflush(stdout);
    fflush(stderr);
    return;
}

void beforeCtrlC(int s) {
    flushStreams();
    log::logError("[beforeCtrlC] Сигнал CTRL + C\n");
    log::close();

    // closeDatabaseConnections();

    exit(EXIT_SUCCESS);
}

void serverBacktrace() {
    void* array[20];

    int size = backtrace(array, 20);

    char** strings = backtrace_symbols(array, size);

    if (strings != NULL) {
        for (int j = 0; j < size; j++) {
            log::logError("[serverBacktrace] %s\n", strings[j]);
        }

        free(strings);
    }

    log::logError("[serverBacktrace] --------------------------------------------\n");
}

void beforeUndefinedInstruction(int s) {
    flushStreams();
    log::logError("[beforeUndefinedInstruction] Недопустимая инструкция\n");

    serverBacktrace();
    // closeDatabaseConnections();
    
    log::close();
    exit(EXIT_FAILURE);
}

void beforeFloatPoint(int s) {
    flushStreams();
    log::logError("[beforeFloatPoint] Ошибка с плавающей запятой - переполнение, или деление на ноль\n");

    serverBacktrace();
    // closeDatabaseConnections();
    
    log::close();
    exit(EXIT_FAILURE);
}

void beforeSegmentationFault(int s) {
    flushStreams();
    log::logError("[beforeSegmentationFault] Ошибка доступа к памяти\n");

    serverBacktrace();
    // closeDatabaseConnections();
    
    log::close();
    exit(EXIT_FAILURE);
}

void beforeTerminate(int s) {
    flushStreams();
    log::logError("[beforeTerminate] Запрос на прекращение работы\n");
    log::close();

    // closeDatabaseConnections();

    exit(EXIT_SUCCESS);
}

void beforeAbort(int s) {
    flushStreams();
    log::logError("[beforeAbort] Аварийное завершение\n");

    serverBacktrace();
    // closeDatabaseConnections();

    log::close();
    exit(EXIT_FAILURE);
}

void signalUSR1(int s) {
    flushStreams();

    moduleLoader::reload();

    printf("signalUSR1\n");

    // syslog(LOG_INFO, "Amount queues of thread handlers: %ld", queue_objs->size());

    // int count = 0;
    // int count2 = 0;

    // unordered_map<int, obj*>::iterator it;

    // for (it = objs->begin(); it != objs->end(); ++it) {
    //     if (it->second->polling->user_id) count++;
    //     if (it->second->polling->data->size()) count2++;
    // }

    // syslog(LOG_INFO, "Polling array size: %d", count);
    // syslog(LOG_INFO, "Polling data array size: %d", count2);
}

void sigpipeHandler(int signum) {
    return;
}

int init() {
    signal(SIGPIPE, sigpipeHandler);
    signal(SIGINT,  beforeCtrlC);
    signal(SIGILL,  beforeUndefinedInstruction);
    signal(SIGFPE,  beforeFloatPoint);
    signal(SIGSEGV, beforeSegmentationFault);
    signal(SIGTERM, beforeTerminate);
    signal(SIGHUP,  sigpipeHandler);
    signal(SIGBUS,  beforeSegmentationFault);
    signal(SIGABRT, beforeAbort);
    signal(SIGUSR1, signalUSR1);

    return 0;
}

} // namespace
