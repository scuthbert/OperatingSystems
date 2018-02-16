#include "queue.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Thread-Safe Queue Operations
int q_init(queue *q, int size, int return_nil, int maxNodeSize) {
  q->bottom = 0;
  q->maxSize = size;
  q->emptyable = return_nil;
  q->array = malloc(sizeof(queue_node) * ((q->maxSize) + 1));

  sem_t lock;
  sem_init(&lock, 0, 1);
  q->lock = lock;

  sem_t waitW;
  sem_init(&waitW, 0, 0);
  q->waiting_writers = waitW;

  sem_t waitR;
  sem_init(&waitR, 0, 0);
  q->waiting_readers = waitR;

  if (!(q->array)) {
    return -1;
  }

  for (int i = 0; i < q->maxSize; i++) {
    q->array[i].data = malloc(sizeof(char) * maxNodeSize + 1);
  }

  return 0;
}

int q_push(queue *q, char *new_data) {
  // Get write permission
  while (1) {
    sem_wait(&(q->lock));

    if (q->bottom == q->maxSize - 1) {
      sem_post(&(q->lock));
      sem_wait(&(q->waiting_writers));
    }

    else {
      strcpy(q->array[q->bottom].data, new_data);

      // fprintf(stderr, "Queue bottom: %d.\n", q->bottom);
      q->bottom++;

      sem_post(&(q->waiting_readers));
      sem_post(&(q->lock));
      return 1;
    }
  }
}

void q_done(queue *q) {
  sem_wait(&(q->lock));
  q->emptyable = 1;
  sem_post(&(q->lock));
}

int q_pop(queue *q, char *dest) {

  while (1) {
    // Get read permissions
    sem_wait(&(q->lock));

    // We're empty
    if (q->bottom == 0) {
      sem_post(&(q->lock)); // Allow others to check
      if (q->emptyable) {
        return -1; // Return if being empty is allowed
      }
      sem_wait(&(q->waiting_readers)); // Else, wait for someone to push
    }

    // We're not full
    else {
      strcpy(dest, q->array[0].data); // Return value

      for (int i = 0; i < q->bottom - 1; i++) {
        strcpy(
            q->array[i].data,
            q->array[i + 1].data);
      }

      q->bottom--; // We're one smaller

      sem_post(&(q->waiting_writers)); // Let someone know they can write
      sem_post(&(q->lock));
      return 1;
    }
  }
}

void q_delete(queue *q) {
  sem_wait(&(q->lock));

  while (q->bottom > 0) {
    q->bottom--;
  }

  for (int i = 0; i < q->maxSize; i++) {
    free(q->array[i].data);
  }

  free(q->array);
  // Don't free up q->lock, freeze access.
}
