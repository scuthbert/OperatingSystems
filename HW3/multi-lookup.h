#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_INPUT_FILES 10
#define MAX_RESOLVE_THREADS 10
#define MAX_REQUEST_THREADS 10
#define MAX_NAME_LENGTH 1025
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
    int emptyable;
    sem_t lock;
    sem_t waiting_writers;
    sem_t waiting_readers;
} queue;

int q_init(queue* q, int size, int return_nil);
int q_push(queue* q, char* payload);
void q_done(queue* q);
int q_pop(queue* q, char* dest);
void q_delete(queue* q);

struct requester_thread {
	  queue* output;
    int* logfile;
    int* done;
    queue* namequeue;
    sem_t* log_sem;
    sem_t* err_sem;
};

struct resolver_thread {
	  queue* input;
    int* logfile;
    int* can_quit;
    int* done;
    sem_t* log_sem;
    sem_t* err_sem;
};

void* request_thread(void* info);
void* resolve_thread(void* info);
