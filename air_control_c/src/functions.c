#include "../include/functions.h"

#define N 3
#define SIZE N * sizeof(int)

#define SHM_NAME "/shm_pids_"

int planes = 0;
int takeoffs = 0;
int total_takeoffs = 0;
int* array_mmap = NULL;
int TOTAL_TAKEOFFS = 20;
int shm_block;

void MemoryCreate() {
  shm_block = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  ftruncate(shm_block, SIZE);
  array_mmap = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_block, 0);

  if (array_mmap == MAP_FAILED) {
    perror("mmap failed");
    exit(1);
  }

  array_mmap[0] = getpid();
  return;
}

void SigHandler2(int signal) {
  if (signal == SIGUSR2) {
    planes += 5;
  }
}

void* TakeOffsFunction(void* arg) {
  while (total_takeoffs < TOTAL_TAKEOFFS) {
    int track1 = pthread_mutex_trylock(&track1_lock);
    int track2 = -1;
    
    if (track1 != 0) {
        // Track 1 failed, try track 2
        track2 = pthread_mutex_trylock(&track2_lock);
    }
    
    // If both failed, sleep and retry
    if (track1 != 0 && track2 != 0) {
        usleep(1000);
        continue;
    }
    
    // We got a track! Figure out which one
    pthread_mutex_t* available;
    if (track1 == 0) {
        available = &track1_lock;
    } else {
        available = &track2_lock;
    }
    
    // Check if planes are available
    pthread_mutex_lock(&state_lock);
    if (planes <= 0) {
        // No planes available, release everything and retry
        pthread_mutex_unlock(&state_lock);
        pthread_mutex_unlock(available);
        usleep(1000);
        continue;
    }
    
    // Planes available, proceed with takeoff
    planes -= 1;
    takeoffs += 1;
    total_takeoffs += 1;
    
    
    if (takeoffs == 5) {
        kill(array_mmap[1], SIGUSR1);
        takeoffs = 0;
    }
    pthread_mutex_unlock(&state_lock);
    
    sleep(1);  // Simulate takeoff time
    pthread_mutex_unlock(available);
  }
  
  // Only one thread should send SIGTERM
  pthread_mutex_lock(&state_lock);
  static int signals_sent = 0;
  if (!signals_sent) {
      usleep(100000);  // Wait for last SIGUSR1 to be processed
      kill(array_mmap[1], SIGTERM);   // Send to radio
      kill(array_mmap[2], SIGTERM);   // Send to ground_control
      signals_sent = 1;
  }
  pthread_mutex_unlock(&state_lock);
  
  return NULL;
}
