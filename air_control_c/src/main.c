#include "../include/functions.h"

#define SHM_NAME "/shm_pids_"
#define SIZE 3*sizeof(int)

pthread_mutex_t state_lock, track1_lock, track2_lock;

int main() {
  // Create the shared memory segment
  MemoryCreate();

  pid_t air_ctrl_pid = getpid();
  pid_t radio_pid;
  pid_t ground_ctrl_pid;

  // Configure the SIGUSR2 signal to increment the planes on the runway by 5
  struct sigaction sa;
  sa.sa_handler = SigHandler2;
  sigaction(SIGUSR2, &sa, NULL);

  // Launch the 'radio' executable
  pid_t radio = fork();

  if (radio == 0) {
    array_mmap[1] = getpid();
    execlp("./radio", "radio", SHM_NAME, NULL);
    perror("execlp failed");
    exit(1);
  }


  // Initialize mutexes
  pthread_mutex_init(&state_lock, NULL);
  pthread_mutex_init(&track1_lock, NULL);
  pthread_mutex_init(&track2_lock, NULL);

  // Launch 5 threads which will be the controllers
  pthread_t t1, t2, t3, t4, t5;

  pthread_create(&t1, NULL, TakeOffsFunction, NULL);
  pthread_create(&t2, NULL, TakeOffsFunction, NULL);
  pthread_create(&t3, NULL, TakeOffsFunction, NULL);
  pthread_create(&t4, NULL, TakeOffsFunction, NULL);
  pthread_create(&t5, NULL, TakeOffsFunction, NULL);

  // Wait for all threads to finish
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  pthread_join(t3, NULL);
  pthread_join(t4, NULL);
  pthread_join(t5, NULL);

  // Wait for radio to finish
  waitpid(radio, NULL, 0);

  // Clean up
  munmap(array_mmap, SIZE);
  close(shm_block);
  shm_unlink(SHM_NAME);
  
  pthread_mutex_destroy(&state_lock);
  pthread_mutex_destroy(&track1_lock);
  pthread_mutex_destroy(&track2_lock);

  return 0;
}
