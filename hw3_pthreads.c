#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>

void *partial_sum(void *);

int *A;	// global variable for array
int num_threads = 5;

struct thread_info_struct {
	pthread_t thread_id;	// id returned by pthread_create
	int num;		// application defined thread id
	int start;		// start index of chunk for thread num
	int end;		// end index of chunk for thread num
};

void main() {
	/* assume thread attributes are initialized here */
	struct thread_info_struct thread_info[num_threads];
	A = (int *) malloc(1000*sizeof(int));
	assert (A != NULL);

	//initialize array
	for (int i = 0; i < 1000; i++)
		A[i] = rand() % 1000;

	double average = 0;
	int* local_sum = (int *)malloc(num_threads*sizeof(int));
	assert(local_sum != NULL);

	for (int i = 0; i < num_threads; i++) {
		thread_info[i].num = i;
		thread_info[i].start = (1000 * i)/num_threads;
		thread_info[i].end = (1000 * (i+1))/num_threads;
		pthread_create(&thread_info[i].thread_id, NULL, &partial_sum, (void *) &thread_info[i]);
	}

	/* join threads */
	int total_sum = 0;
	for (int i = 0; i < num_threads; i++) {
		void *thread_val;
		pthread_join(thread_info[i].thread_id, &thread_val);
		local_sum[thread_info[i].num] = *((int *) thread_val);
		total_sum += local_sum[thread_info[i].num];
	}
	
	average = total_sum/1000.0;
	free(local_sum);
}

void *partial_sum(void *myinfo) {
	struct thread_info_struct *thread_info = (struct thread_info_struct *) myinfo;

	int *sum = (int *) malloc(sizeof(int));
	*sum = 0;
	for (int i = thread_info -> start; i < thread_info -> end; i++) {
		*sum += A[i];
	}

	return sum;
}
