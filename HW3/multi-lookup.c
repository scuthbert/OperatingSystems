#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include "util.h"
#include "multi-lookup.h"

int main(int argc, char * argv[]){
	// Initial timing variables
	struct timeval t0;
	gettimeofday(&t0, 0);
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
  int results = open(argv[3], O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
  if (results < 0) {
    fprintf(stderr, "Fatal error: Cannot write to %s\n", argv[3]);
    fprintf(stderr, "Usage:\n %s %s\n", argv[0], HOWTO);
    return -1;
  }

  // See if serviced file is writeable
  int serviced = open(argv[4], O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
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
  */
  sem_t results_sem;
  sem_init(&results_sem, 0, 1);
  sem_t serviced_sem;
  sem_init(&serviced_sem, 0, 1);
  sem_t stderr_sem;
  sem_init(&stderr_sem, 0, 1);

  // Create a shared array
  queue shared_arr;
  if(q_init(&shared_arr, 32, 0, MAX_NAME_LENGTH) == -1) {
      fprintf(stderr, "Fatal Error: Could not allocate shared array.\n");
      return -1;
  }

  // Create queue of name files
  queue namefiles;
  if(q_init(&namefiles, MAX_INPUT_FILES + 1, 1, MAX_NAME_LENGTH) == -1) {
      fprintf(stderr, "Fatal Error: Could not allocate namefile queue.\n");
      return -1;
  }

  for(int i = 5; i < argc; i++){
    q_push(&namefiles, argv[i]);
  }

	int requesters_done = 0;
	int can_resolvers_quit = 0;
	int resolvers_done = 0;

  // Spin up requester threads
  pthread_t requesters[atoi(argv[1])];
  struct requester_thread request_info;
  request_info.output = &shared_arr;
  request_info.logfile = &serviced;
  request_info.done = &requesters_done;
  request_info.log_sem = &serviced_sem;
  request_info.err_sem = &stderr_sem;
  request_info.namequeue = &namefiles;

  for(int i = 0; i < atoi(argv[1]); i++) {
		pthread_create(&requesters[i], NULL, request_thread, &request_info);
  }

	// Spin up requester threads
	pthread_t resolvers[atoi(argv[2])];
	struct resolver_thread resolve_info;
	resolve_info.input = &shared_arr;
	resolve_info.logfile = &results;
	resolve_info.can_quit = &can_resolvers_quit;
	resolve_info.done = &resolvers_done;
	resolve_info.log_sem = &results_sem;
	resolve_info.err_sem = &stderr_sem;

	for(int i = 0; i < atoi(argv[2]); i++) {
		pthread_create(&resolvers[i], NULL, resolve_thread, &resolve_info);
	}

	// Wait for all requesters to exit
	for(int i = 0; i < atoi(argv[1]); i++) {
		pthread_join(requesters[i], NULL);
	}

	// Allow resolvers to exit
	sem_wait(&results_sem);
	q_done(&shared_arr);
	can_resolvers_quit = 1;
	sem_post(&results_sem);

	// Wait for all resolvers to exit
	for(int i = 0; i < atoi(argv[2]); i++) {
		pthread_join(resolvers[i], NULL);
	}

  // Clean up shared array
  q_delete(&namefiles);
  q_delete(&shared_arr);

	// Finish timing
	struct timeval t1;
	gettimeofday(&t1, 0);
	double elapsed = ((t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec);
	fprintf(stdout, "Time Elapsed: %.6e microseconds.\n", elapsed);

	return 0;
}

void* request_thread(void* id){
  struct requester_thread* info = id;
  int filesServed = 0;
  int err = 0;

	char* namefile = malloc(sizeof(char) * MAX_NAME_LENGTH + 1);
	int done = q_pop(info->namequeue, namefile);

	// While there are still files to be served
	while(done != -1) {
    filesServed++;
    FILE* file = fopen(namefile, "r");
    char line[MAX_NAME_LENGTH + 1]; // 1 for null char at EOS

    //Read off every line in file
    while(fgets(line, sizeof(line), file) != NULL) {
			line[strcspn(line, "\n")] = 0; // Trim newline
			q_push(info->output, line); // Keep pushing: Our queue
														 		  // 	DS will prevent busy wait.
    }

		// All done with this file
    fclose(file);

		// Reset name
		memset(namefile, '\0', sizeof(*namefile));
		done = q_pop(info->namequeue, namefile);
  }

  // Log what we did
  char* outputline = malloc(33 * sizeof(char) + 1);
  if(sprintf(outputline, "Thread %lu serviced %03d files.\n", syscall(SYS_gettid), filesServed) < 0) err++;

	sem_wait(info->log_sem);
	if(write(*(info->logfile), outputline, strlen(outputline)) != (long)strlen(outputline)) err++;
  sem_post(info->log_sem);

	// Wrap up things
	free(outputline);
	free(namefile);

  // Tell the console we're exiting
	sem_wait(info->err_sem);
  fprintf(stderr, "Thread %lu exiting, failures: %d \n", syscall(SYS_gettid), err);
  sem_post(info->err_sem);

  return id;
}

void* resolve_thread(void* id){
	struct resolver_thread* info = id;
	int err = 0;
	int can_quit = 0;

	char* domain = malloc(sizeof(char) * MAX_NAME_LENGTH + 1);

	// While there are still files to be served
	while(1) {
		// Check if we can quit
		sem_wait(info->log_sem);
		can_quit = *(info->can_quit);
		sem_post(info->log_sem);

		// Reset domain
		memset(domain, '\0', sizeof(*domain));
		int done = q_pop(info->input, domain);

		if(can_quit && done == -1) {
			// Free things up
			free(domain);

			// Tell the console we're exiting
			sem_wait(info->err_sem);
			fprintf(stderr, "Resolver %lu exiting, failures: %d \n", syscall(SYS_gettid), err);
			sem_post(info->err_sem);

			return id;
		} else {
			//IP Address
			char ipaddr[MAX_IP_LENGTH + 1];

			// Resolve domain
			if(dnslookup(domain, ipaddr, sizeof(ipaddr)) == -1) {
				// We failed
				strncpy(ipaddr, "", sizeof(ipaddr));
				err++;
			}

			// Output to file
			char line[MAX_NAME_LENGTH + MAX_IP_LENGTH + 2]; // +2 = 1 for comman, 1 for EOS
		  if(sprintf(line, "%s,%s\n", domain, ipaddr) < 0) err++;

			sem_wait(info->log_sem);
			if(write(*(info->logfile), line, strlen(line)) != (long)strlen(line)) err++;
			sem_post(info->log_sem);
		}
	}
}
