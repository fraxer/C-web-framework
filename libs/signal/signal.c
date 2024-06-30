#include <stdlib.h>
#include <execinfo.h>
#include <stdio.h>
#include <signal.h>

#include "log.h"
#include "moduleloader.h"
#include "signal.h"

void signal_flush_streams() {
    fflush(stdout);
    fflush(stderr);
    return;
}

void signal_before_ctrl_c(int s) {
    (void)s;
    signal_flush_streams();
    log_error("[signal_before_ctrl_c] Сигнал CTRL + C\n");
    log_close();

    // close_database_connections();

    exit(EXIT_SUCCESS);
}

void server_backtrace() {
    void* array[20];

    int size = backtrace(array, 20);

    char** strings = backtrace_symbols(array, size);

    if (strings != NULL) {
        for (int j = 0; j < size; j++) {
            printf("[server_backtrace] %s\n", strings[j]);
        }

        free(strings);
    }

    log_error("[server_backtrace] --------------------------------------------\n");
}

void signal_before_undefined_instruction(int s) {
    (void)s;
    signal_flush_streams();
    log_error("[signal_before_undefined_instruction] Недопустимая инструкция\n");

    server_backtrace();
    // close_database_connections();
    
    log_close();
    exit(EXIT_FAILURE);
}

void signal_before_float_point(int s) {
    (void)s;
    signal_flush_streams();
    log_error("[signal_before_float_point] Ошибка с плавающей запятой - переполнение, или деление на ноль\n");

    server_backtrace();
    // close_database_connections();
    
    log_close();
    exit(EXIT_FAILURE);
}

void signal_before_segmentation_fault(int s) {
    (void)s;
    signal_flush_streams();
    log_error("[signal_before_segmentation_fault] Ошибка доступа к памяти\n");

    server_backtrace();
    // close_database_connections();
    
    log_close();
    exit(EXIT_FAILURE);
}

void signal_before_terminate(int s) {
    (void)s;
    signal_flush_streams();
    log_error("[signal_before_terminate] Запрос на прекращение работы\n");
    log_close();

    // close_database_connections();

    exit(EXIT_SUCCESS);
}

void signal_before_abort(int s) {
    (void)s;
    signal_flush_streams();
    log_error("[signal_before_abort] Аварийное завершение\n");

    server_backtrace();
    // close_database_connections();

    log_close();
    exit(EXIT_FAILURE);
}

void signal_USR1(int s) {
    (void)s;
    signal_flush_streams();

    printf("signal_USR1\n");

    module_loader_reload();

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

void signal_sigpipe_handler(int s) {
    (void)s;
    return;
}

int signal_init() {
    signal(SIGPIPE, signal_sigpipe_handler);
    signal(SIGINT,  signal_before_ctrl_c);
    signal(SIGILL,  signal_before_undefined_instruction);
    signal(SIGFPE,  signal_before_float_point);
    signal(SIGSEGV, signal_before_segmentation_fault);
    signal(SIGTERM, signal_before_terminate);
    signal(SIGHUP,  signal_sigpipe_handler);
    signal(SIGBUS,  signal_before_segmentation_fault);
    signal(SIGABRT, signal_before_abort);
    signal(SIGUSR1, signal_USR1);
    signal(SIGUSR2, SIG_IGN);

    return 0;
}
