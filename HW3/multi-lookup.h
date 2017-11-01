#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_INPUT_FILES 10
#define MAX_RESOLVE_THREADS 10
#define MAX_REQUEST_THREADS 10
#define MAX_NAME_LENGTHS 1025
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

#define HOWTO "<# of requester threads> <# of resolver threads> <output file results> <output file serviced> <input files>"
#define MINARGS 6
#define MAXARGS MINARGS + MAX_INPUT_FILES - 1

typedef struct queue_node_struct {
    char* data;
} queue_node;

typedef struct queue_struct {
    queue_node* array;
    int bottom;
    int maxSize;
    sem_t lock;
    sem_t waiting_writers;
} queue;

int q_init(queue* q, int size);
int q_empty(queue* q);
int q_full(queue* q);
int q_push(queue* q, char* payload);
char* q_pop(queue* q);
void q_delete(queue* q);

struct requester_thread {
	  queue* output;
    int* logfile;
    queue* namequeue;
    sem_t* log_sem;
    sem_t* err_sem;
};

struct resolver_thread {
	  queue* input;
    int* logfile;
    sem_t* log_sem;
    sem_t* err_sem;
};

void* request_thread(void* info);
void* resolve_thread(void* info);
