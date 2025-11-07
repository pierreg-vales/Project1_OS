#ifndef INCLUDE_FUNCTIONS_H_
#define INCLUDE_FUNCTIONS_H_

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern int* array_mmap;
extern pthread_mutex_t state_lock, runway1_lock, runway2_lock;
void MemoryCreate();
void SigHandler2(int signal);
void* TakeOffsFunction(void* arg);

#endif  // INCLUDE_FUNCTIONS_H_