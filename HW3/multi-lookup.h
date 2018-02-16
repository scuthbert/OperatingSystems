#include "queue.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_INPUT_FILES 10
#define MAX_RESOLVE_THREADS 10
#define MAX_REQUEST_THREADS 10
#define MAX_NAME_LENGTH 1025
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

#define HOWTO                                                                  \
  "<# of requester threads> <# of resolver threads> <output file results> "    \
  "<output file serviced> <input files>"
#define MINARGS 6
#define MAXARGS MINARGS + MAX_INPUT_FILES - 1

struct requester_thread {
  queue *output;
  int *logfile;
  int *done;
  queue *namequeue;
  sem_t *log_sem;
  sem_t *err_sem;
};

struct resolver_thread {
  queue *input;
  int *logfile;
  int *can_quit;
  int *done;
  sem_t *log_sem;
  sem_t *err_sem;
};

void *request_thread(void *info);
void *resolve_thread(void *info);
