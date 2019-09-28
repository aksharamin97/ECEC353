//To compile: gcc -o hw5 ama399_hw5.c -pthread

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#define NUM_WORKER_THREADS 20

struct thread_data {
	pthread_t tid;
	unsigned int num;
};

struct workers_state {
	int still_working;
	pthread_mutex_t mutex;
	pthread_cond_t signal;
};

static struct workers_state wstate = {
	.still_working = NUM_WORKER_THREADS,
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.signal = PTHREAD_COND_INITIALIZER
};

static unsigned int result[NUM_WORKER_THREADS];

void* worker_thread (void* param)
{
	struct thread_data* thread = (struct thread_data*) param;
	result[thread->num] = thread->num*thread->num; 	
	pthread_mutex_lock(&wstate.mutex);
	wstate.still_working--;

	pthread_mutex_unlock(&wstate.mutex);
	pthread_cond_broadcast(&wstate.signal);
	pthread_exit(NULL);
}

int main (int argc, char** argv)
{
	int i;
	struct thread_data* threads = malloc(sizeof(struct thread_data)*NUM_WORKER_THREADS);
	int total = 0;
	int c = 0;
	
	for (i = 0; i < NUM_WORKER_THREADS; i++){
		threads[i].num = i;
		pthread_create(&threads[i].tid, NULL, worker_thread, &threads[i]);
		pthread_detach(threads[i].tid);
	}
	pthread_mutex_lock(&wstate.mutex);
	if(wstate.still_working == 1){
		pthread_cond_wait(&wstate.signal, &wstate.mutex);
	}
	pthread_mutex_unlock(&wstate.mutex);
	
	for (i = 0; i < NUM_WORKER_THREADS; i++){
		total = total + result[i];
		c = c + i * i;
	}
	printf("Total = %i\n",total);
	printf("C = %i\n", c);
}
