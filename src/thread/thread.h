#ifndef __THREAD__
#define __THREAD__

void* thread_init();

void* thread_worker(void* arg);

void* thread_handler(void* arg);

#endif
