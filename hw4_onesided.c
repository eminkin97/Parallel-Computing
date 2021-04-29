#include <stdio.h>
#include <string.h>
#include <math.h>

#include "mpi.h"

struct tuple {
	int key;	// integer key
	val value;	// some k-byte type val
};

int comparator(void *a, void *b) {
	struct tuple *c = (struct tuple *) a;
	struct tuple *d = (struct tuple *) b;
	return c->key - d->key;
}

int main(int argc, char *argv) {
	// get rank and size in MPI grid
	int rank, size; 
	MPI_Init(&argc, &argv); 
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size); 
	
	//each process has an array of tuples (how it reads it is omitted)
	int numtuples = ...
	struct tuple *arr = ...

	// get counts for each key process has
	int *keycounts = (int *) malloc(sizeof(int) * size);
	memset(keycounts, 0, size);		//zero the array
	for (int i = 0; i < numtuples; i++) {
		keycounts[arr[i].key]++;
	}
	
	// sort array arr based on tuple key using library qsort
	qsort(arr, numtuples, sizeof(struct tuple), comparator);
	
	// calculate starting position in arr by calculating prefix sum of local keycounts
	int *startingposition = (int *) malloc(sizeof(int) * size);
	startingposition[0] = 0;
	for (int i = 1; i < size; i++) {
		startingposition[i] = startingposition[i-1] + keycounts[i-1];
	}
	
	// get total count for all keys using AllReduce
	int *totalcounts = (int *) malloc(sizeof(int) * size);
	MPI_Allreduce(keycounts, totalcounts, size, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
	
	// do a scan (parallel prefix) on all process-wise keycounts
	int *recvcounts = (int *) malloc(sizeof(int) * size);
	MPI_Exscan(keycounts, recvcounts, size, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
	
	//use the received parallel prefix counts to send tuples to correct processes
	//create a window where other processes can put the tuples they are sending to this process
	MPI_Win win;
	struct tuple *receivedtuples = (struct tuple *) malloc(sizeof(struct tuple) * totalcounts[rank]);
	MPI_Win_create(receivedtuples, totalcounts[rank]*sizeof(struct tuple), sizeof(struct tuple), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
	
	// put tuples to their appropriate windows
	MPI_Win_fence(0, win);	// fence to ensure completion of MPI_Put
	for (int i = 0; i < size; i++) {
		MPI_Put(&arr[startingposition[i]], sizeof(struct tuple) * keycounts[i], MPI_Byte, i, recvcounts[i], sizeof(struct tuple) * keycounts[i], MPI_Byte, win);
	}
	MPI_Win_fence(0, win);
	
	// after end of fence, receivedtuples array will have the desired tuples for this process
	// put into val array
	val *finalarr = (val *) malloc(sizeof(val) * totalcounts[rank]);
	for (int i = 0; i < totalcounts[rank]; i++) {
		finalarr[i] = receivedtuples[i].value;
	}

	//free malloc'ed memory
	free(keycounts);
	free(startingposition);
	free(totalcounts);
	free(recvcounts);
	free(receivedtuples);
	free(finalarr);

	//Finalize
	MPI_Finalize();
	return 0;
}