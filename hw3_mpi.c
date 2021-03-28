#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <mpi.h>

void my_reduce(int *my_val, int *sum_val) {
	int myrank, size;
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	MPI_Comm_size(MPI_COMM_WORLD,&size);
	MPI_Status leftstatus, rightstatus;
	// check if youre a leaf node
	if (myrank * 2 + 1 >= size) {
		MPI_Send(my_val,1,MPI_INT,(myrank-1)/2,0,MPI_COMM_WORLD);
	} else {
		// receive from your left child
		int leftchild;
		MPI_Recv(&leftchild,1,MPI_INT,myrank*2+1,0,MPI_COMM_WORLD, &leftstatus);

		// receive from your right child
		int rightchild;
		if (myrank * 2 + 2 < size) {
			MPI_Recv(&rightchild,1,MPI_INT,myrank*2+2,0,MPI_COMM_WORLD, &rightstatus);
		}

		//send to parent
		*sum_val = *my_val + leftchild + rightchild;
		if (myrank != 0) {
			MPI_Send(sum_val,1,MPI_INT,(myrank-1)/2,0,MPI_COMM_WORLD);
		} else {
			*sum_val = 0;
		}
	}

	if (myrank == 0)
		printf("%d\n", *sum_val);
}

int main(int argc, char *argv[]) {

	MPI_Init(&argc, &argv);
	int *my_val = (int *) malloc(sizeof(int));
	int *sum_val = (int *) malloc(sizeof(int));
	*my_val = 1;
	my_reduce(my_val, sum_val);
	MPI_Finalize();
	return 0;
}
