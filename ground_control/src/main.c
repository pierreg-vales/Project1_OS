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
#include <sys/time.h>

#define SHM_NAME "/shm_pids_"
#define PLANES_LIMIT 20
#define N 3
#define SIZE N*sizeof(int)

int planes = 0;
int takeoffs = 0;
int fd;
int *arr_mmap;

void SigtermHandler(int signal){
  if (signal == SIGTERM){
    munmap(arr_mmap, SIZE);
    close(fd);
    printf("finalization of operations...\n");
    exit(0);
  }
}

void Sigusr1Handler(int signal){
  if (signal == SIGUSR1){
    takeoffs += 5;
    return;
  }
}

void Traffic(int signum) {
  int waiting = planes - takeoffs;
  
  if (waiting >= 10){
    printf("RUNWAY OVERLOADED\n");
  }

  if (planes < PLANES_LIMIT){
    planes += 5;
    kill(arr_mmap[1], SIGUSR2);
  }
}

int main(int argc, char* argv[]) {
  // Open the shared memory block
  fd = shm_open(SHM_NAME, O_RDWR, 0666);
  if (fd == -1) {
    perror("shm_open");
    exit(1);
  }
  
  // Map shared memory
  arr_mmap = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (arr_mmap == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }
  
  // Store this process PID in position 2 of the memory block
  arr_mmap[2] = getpid();

  // Configure SIGTERM and SIGUSR1 handlers
  struct sigaction sa1, sa2;
  sa1.sa_handler = SigtermHandler;
  sigaction(SIGTERM, &sa1, NULL);
  sa2.sa_handler = Sigusr1Handler;
  sigaction(SIGUSR1, &sa2, NULL);

  // Configure the timer to execute the Traffic function every 500ms
  struct itimerval timer;
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 500000;  // 500ms
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 500000;  // Repeat every 500ms
    
  signal(SIGALRM, Traffic);
  setitimer(ITIMER_REAL, &timer, NULL);

  // Keep running until SIGTERM is received
  while (1) {
    pause();
  }

  return 0;
}
