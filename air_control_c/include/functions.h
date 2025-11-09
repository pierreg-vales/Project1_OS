#ifndef FUNCTIONS_H
#define FUNCTIONS_H

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

// Global variables
extern int planes;
extern int takeoffs;
extern int total_takeoffs;
extern int* array_mmap;
extern int TOTAL_TAKEOFFS;
extern int shm_block;

// Mutexes
extern pthread_mutex_t state_lock, track1_lock, track2_lock;

// Functions
void MemoryCreate();
void SigHandler2(int signal);
void* TakeOffsFunction(void* arg);

#endif