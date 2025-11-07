#include "../include/functions.h"

#define N 3
#define SIZE N * sizeof(int)

#define SHM_NAME "/shm_pid"

int planes = 0;
int takeoffs = 0;
int total_takeoffs = 0;
int* array_mmap = NULL;
int TOTAL_TAKEOFFS = 20;

void MemoryCreate() {
  // TODO2: create the shared memory segment, configure it and store the PID of
  // the process in the first position of the memory block.

  int shm_block = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  ftruncate(shm_block, SIZE);
  array_mmap = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_block, 0);

  if (array_mmap == MAP_FAILED) {
    perror("mmap failed");
    exit(1);
  }

  array_mmap[0] = getpid();
}

void SigHandler2(int signal) {
  if (signal == SIGUSR2) {
    planes += 5;
  }
}

void* TakeOffsFunction(void* arg) {
  // TODO: implement the logic to control a takeoff thread
  //    Use a loop that runs while total_takeoffs < TOTAL_TAKEOFFS
  //    Use runway1_lock or runway2_lock to simulate a locked runway
  //    Use state_lock for safe access to shared variables (planes,
  //    takeoffs, total_takeoffs)
  //    Simulate the time a takeoff takes with sleep(1)
  //    Send SIGUSR1 every 5 local takeoffs
  //    Send SIGTERM when the total takeoffs target is reached
}