#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "util.h"
#include "multi-lookup.h"

int main(int argc, char * argv[]){
  // Let's validate those command line arguments!
  // See if we have enough arguments
	if(argc < MINARGS) {
		fprintf(stderr, "Fatal Error: You need at least %d input files to run this program \n", MINARGS);
    fprintf(stderr, "Usage:\n %s %s\n", argv[0], HOWTO);
		return -1;
	}

  // See if we have too many arguments
  if(argc > MAXARGS) {
    fprintf(stderr, "Fatal Error: You can use at most %d input files to run this program \n", MAX_INPUT_FILES);
    fprintf(stderr, "Usage:\n %s %s\n", argv[0], HOWTO);
    return -1;
  }

  // See if we have a positive int for requester threads
  if(atoi(argv[1]) < 1 || atoi(argv[1]) > MAX_REQUEST_THREADS) {
      fprintf(stderr, "Fatal error: Number of requester threads must be positive and less than %d.\n", MAX_REQUEST_THREADS + 1);
      fprintf(stderr, "Usage:\n %s %s\n", argv[0], HOWTO);
      return -1;
  }

  // See if we have a positive int for resolver threads
  if(atoi(argv[2]) < 1 || atoi(argv[2]) > MAX_RESOLVE_THREADS) {
      fprintf(stderr, "Fatal error: Number of resolver threads must be positive and less than %d.\n",  MAX_RESOLVE_THREADS + 1);
      fprintf(stderr, "Usage:\n %s %s\n", argv[0], HOWTO);
      return -1;
  }

  // See if results file is writeable
  int results = open(argv[3], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
  if (results < 0) {
    fprintf(stderr, "Fatal error: Cannot write to %s\n", argv[3]);
    fprintf(stderr, "Usage:\n %s %s\n", argv[0], HOWTO);
    return -1;
  }

  // See if serviced file is writeable
  int serviced = open(argv[4], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
  if (serviced < 0) {
    fprintf(stderr, "Fatal error: Cannot write to %s\n", argv[4]);
    fprintf(stderr, "Usage:\n %s %s\n", argv[0], HOWTO);
    return -1;
  }

  // See if all name files are readable
  for(int i = 5; i < argc; i++) {
    if(access(argv[i], R_OK) == -1) {
      fprintf(stderr, "Fatal Error: Name file %s not readable.\n", argv[i]);
      return -1;
    }
  }

  /*
  * At this point, we can start actually doing things. Let's start by declaring
  *   some semaphores to control access to:
  *     - results file
  *     - serviced file
  *     - stderr
  *     - shared array: Let's take care of this in the queue functions
  *   These two noly if we need it (num of requesters < num of name files)
  *     - getting new name file
  *     - need a new name file
  */
  sem_t results_sem;
  sem_init(&results_sem, 0, 1);
  sem_t serviced_sem;
  sem_init(&serviced_sem, 0, 1);
  sem_t stderr_sem;
  sem_init(&stderr_sem, 0, 1);

  // Create a shared array
  queue shared_arr;
  if(q_init(&shared_arr, 1024) == -1) {
      fprintf(stderr, "Fatal Error: Could not allocate shared array.\n");
      return -1;
  }

  // Create queue of name files
  queue namefiles;
  if(q_init(&namefiles, MAX_INPUT_FILES) == -1) {
      fprintf(stderr, "Fatal Error: Could not allocate namefile queue.\n");
      return -1;
  }
  //q_init(&namefiles, MAX_INPUT_FILES);
  for(int i = 5; i < argc; i++){
    q_push(&namefiles, argv[i]);
  }

  for(int i = 5; i < argc; i++){
    fprintf(stdout, "Namefile: %s\n", q_pop(&namefiles));
  }

  // Spin up requester threads
  pthread_t requesters[atoi(argv[1])];
  struct requester_thread info;
  info.output = &shared_arr;
  info.logfile = &serviced;
  info.log_sem = &serviced_sem;
  info.err_sem = &stderr_sem;
  info.namequeue = &namefiles;

  for(int i = 0; i < atoi(argv[1]); i++){
		pthread_create(&requesters[i], NULL, request_thread, &info);
  }
  sleep(10);

  // Clean up shared array
  q_delete(&namefiles);
  q_delete(&shared_arr);
  return 0;
}

void* request_thread(void* id){
  struct requester_thread* info = id;
  sem_wait(info->err_sem);
  fprintf(stderr, "Hello from thread #:%lu\n", syscall(SYS_gettid));
  sem_post(info->err_sem);

  return id;
}

void* resolve_thread(void* id){
  return id;
/*
  struct thread* thread = id; //Make a thread to hold info
  char* domain; //domain char arrays
	FILE* outfile = thread->thread_file; //Output file
	pthread_mutex_t* buffmutex = thread->buffmutex; //Buffer mutex
	pthread_mutex_t* outmutex = thread->outmutex; //Output mutex
	queue* buffer = thread->buffer; //Queue
	char ipstr[MAX_IP_LENGTH]; //IP Addresses
    while(!queue_is_empty(buffer) || requests_exist){ //while the queue has stuff or there's request threads, loop
		pthread_mutex_lock(buffmutex); //lock buffer
		domain = queue_pop(buffer); //pop off queue
		if(domain == NULL){ //if empty, unlock
			pthread_mutex_unlock(buffmutex);
			usleep(rand() % 100 + 5);
		}
		else { //Unlock and go!
			pthread_mutex_unlock(buffmutex);
			if(dnslookup(domain, ipstr, sizeof(ipstr)) == UTIL_FAILURE)//look up domain, or try
				strncpy(ipstr, "", sizeof(ipstr));
			printf("%s:%s\n", domain, ipstr);
            pthread_mutex_lock(outmutex); //lock output file, if possible
			fprintf(outfile, "%s,%s\n", domain, ipstr); //write to output file
			pthread_mutex_unlock(outmutex); //unlock output, if possible
    	}
			free(domain);
	}
    return NULL; */
}

// Thread-Safe Queue Operations
int q_init(queue* q, int size) {
  q->bottom = 0;
  q->maxSize = size;

  q->array = malloc(sizeof(queue_node) * (q->maxSize));

  sem_t lock;
  sem_init(&lock, 0, 1);
  q->lock = lock;

  sem_t waitR;
  sem_init(&waitR, 0, 0);
  q->waiting_writers = waitR;

  if(!(q->array)) {
    return -1;
  }

  for(int i = 0; i < q->maxSize; i++){
    q->array[i].data = NULL;
  }

  return 0;
}

int q_empty(queue* q){
  // Get read permission
  sem_wait(&(q->lock));
  if(q->bottom == 0){
    sem_post(&(q->lock));
	  return 1;
  } else {
    sem_post(&(q->lock));
	  return 0;
  }
}

int q_full(queue* q){
  // Get read permission
  sem_wait(&(q->lock));
  if(q->bottom == q->maxSize){
    sem_post(&(q->lock));
	  return 1;
  } else {
    sem_post(&(q->lock));
	  return 0;
  }
}

int q_push(queue* q, char* new_data) {
  // Get write permission
  sem_wait(&(q->lock));
  if(q->bottom == q->maxSize) {
    sem_post(&(q->lock));
    sem_wait(&(q->waiting_writers));
    return -1;
  } else {
    q->array[q->bottom].data = new_data;
    q->bottom++;
    sem_post(&(q->lock));
    return 1;
  }
}

char* q_pop(queue* q) {
  // Get read permission
  sem_wait(&(q->lock));
  if(q->bottom == 0) {
    sem_wait(&(q->lock));
    return NULL;
  } else {
    char* data = q->array[0].data;
    for(int i = 0; i <= q->bottom; i++) {
      q->array[i].data = q->array[i + 1].data;
    }
    q->bottom--;
    sem_post(&(q->waiting_writers));
    sem_post(&(q->lock));
    return data;
  }
}

void q_delete(queue* q) {
  sem_wait(&(q->lock));

  while(q->bottom > 0) {
    fprintf(stderr, "%s", q->array[q->bottom].data);
    q->bottom--;
  }

  free(q->array);
  // Don't free up q->lock, freeze access.
}
