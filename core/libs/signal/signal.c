#define _GNU_SOURCE
#include <stdlib.h>
#include <execinfo.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <libgen.h>
#include <linux/limits.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <socket.h>
#include <dirent.h>
#include <string.h>

#include "log.h"
#include "moduleloader.h"
#include "signal.h"
#include "appconfig.h"

int wait = 0;

static void __signal_shutdown_sockets(void);
static int __signal_is_inet_socket(int fd);

void signal_flush_streams() {
    fflush(stdout);
    fflush(stderr);
    return;
}

void print_stack_trace(void) {
    void* array[100];
    size_t size;

    // Get the stack trace
    size = backtrace(array, 100);
    if (size == 0) {
        fprintf(stderr, "Error getting stack trace\n");
        return;
    }

    // Print the stack trace
    char** symbols = backtrace_symbols(array, size);
    if (symbols == NULL) {
        fprintf(stderr, "Error printing stack trace\n");
        return;
    }

    for (size_t i = 0; i < size; i++)
        printf("%s\n", symbols[i]);

    #pragma GCC diagnostic ignored "-Wanalyzer-unsafe-call-within-signal-handler"
    free(symbols);
}

void signal_before_ctrl_c(__attribute__((unused))int s) {
    signal_flush_streams();
    log_error("[signal_before_ctrl_c] Сигнал CTRL + C\n");
}

void signal_before_undefined_instruction(__attribute__((unused))int s) {
    signal_flush_streams();
    log_error("[signal_before_undefined_instruction] Недопустимая инструкция\n");
}

void signal_before_float_point(__attribute__((unused))int s) {
    signal_flush_streams();
    log_error("[signal_before_float_point] Ошибка с плавающей запятой - переполнение, или деление на ноль\n");
}

void signal_before_segmentation_fault(__attribute__((unused))int s) {
    signal_flush_streams();
    print_stack_trace();
    log_error("[signal_before_segmentation_fault] Ошибка доступа к памяти\n");
}

void signal_before_terminate(__attribute__((unused))int s) {
    signal_flush_streams();
    log_error("[signal_before_terminate] Запрос на прекращение работы\n");
}

void signal_before_abort(__attribute__((unused))int s) {
    signal_flush_streams();
    log_error("[signal_before_abort] Аварийное завершение\n");
}

int __signal_is_inet_socket(int fd) {
    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);

    if (getsockname(fd, (struct sockaddr*)&addr, &len) == -1) {
        log_error("__signal_is_inet_socket: error getsockname %d\n", errno);
        return -1;
    }

    return addr.ss_family == AF_INET || addr.ss_family == AF_INET6;
}

void __signal_shutdown_sockets(void) {
    DIR* dir = opendir("/proc/self/fd");
    if (dir == NULL) return;

    char link_path[32] = {0};
    struct dirent* entry = NULL;
    struct stat sb;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_LNK) continue;

        strcpy(link_path, "/proc/self/fd/");
        strcat(link_path, entry->d_name);

        if (stat(link_path, &sb) != 0) continue;

        if (!S_ISSOCK(sb.st_mode)) continue;

        const int fd = atoi(entry->d_name);
        if (fd <= 2) continue;

        if (__signal_is_inet_socket(fd))
            shutdown(fd, SHUT_RDWR);
    }

    closedir(dir);
}

void signal_USR1(__attribute__((unused))int s) {
    signal_flush_streams();
    log_error("signal USR1\n");

    if (module_loader_signal_locked()) {
        log_info("signal_USR1: wait reload\n");
        return;
    }

    if (module_loader_config_correct(appconfig_path())) {
        module_loader_signal_lock();

        appconfig_t* config = appconfig();
        if (config == NULL) {
            log_error("config is NULL\n");
            module_loader_create_config_and_init();
            return;
        }

        if (config->env.main.reload == APPCONFIG_RELOAD_HARD)
            __signal_shutdown_sockets();

        atomic_store(&config->threads_pause, 1);
    }
}

void signal_init(void) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT,  signal_before_ctrl_c);
    signal(SIGILL,  signal_before_undefined_instruction);
    signal(SIGFPE,  signal_before_float_point);
    signal(SIGSEGV, signal_before_segmentation_fault);
    signal(SIGTERM, signal_before_terminate);
    signal(SIGHUP,  SIG_IGN);
    signal(SIGBUS,  signal_before_segmentation_fault);
    signal(SIGABRT, signal_before_abort);
    signal(SIGUSR1, signal_USR1);
    signal(SIGUSR2, SIG_IGN);
}
