#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>

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

int q_init(queue* q, int size, int return_nil, int maxNodeSize);
int q_push(queue* q, char* payload);
void q_done(queue* q);
int q_pop(queue* q, char* dest);
void q_delete(queue* q);

#endif
