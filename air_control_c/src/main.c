#include "../include/functions.h"

#define SHM_NAME "/shm_pid"

int main() {
  // TODO 1: Call the function that creates the shared memory segment.
  MemoryCreate();

  pid_t air_ctrl_pid = getpid();
  pid_t radio_pid;
  pid_t ground_ctrl_pid;

  // TODO 3: Configure the SIGUSR2 signal to increment the planes on the runway
  // by 5.
  struct sigaction sa;
  sa.sa_handler = SigHandler2;
  sigaction(SIGUSR2, &sa, NULL);

  // TODO 4: Launch the 'radio' executable and, once launched, store its PID in
  // the second position of the shared memory block.
  pid_t radio = fork();

  if (radio == 0) {
    array_mmap[1] = getpid();
    execlp("../build/radio", "./radio", SHM_NAME, NULL);
    perror("execlp failed");
    exit(1);
  }

  // TODO 6: Launch 5 threads which will be the controllers; each thread will
  // execute the TakeOffsFunction().

  pthread_t t1, t2, t3, t4, t5;

  pthread_mutex_init(&state_lock, NULL);

  pthread_create(&t1, NULL, TakeOffsFunction, NULL);
  pthread_create(&t2, NULL, TakeOffsFunction, NULL);
  pthread_create(&t3, NULL, TakeOffsFunction, NULL);
  pthread_create(&t4, NULL, TakeOffsFunction, NULL);
  pthread_create(&t5, NULL, TakeOffsFunction, NULL);
}