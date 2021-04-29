#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <mpi.h>


int main(int argc, char *argv[]) {
	// get rank and size in MPI grid
	int rank, size; 
	MPI_Init(&argc, &argv); 
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size); 
	
	int N; // size of data, NxN matrix
	int *data; // matrix - initial matrix
	int submatrixsize;		// size of submatrix = (N/P)x(N/P)
	// process 0 reads data
	if (rank == 0) {
		//READ DATA
		N = ...;
		data = ...;
		submatrixsize = N/size;
	}
	
	// broadcast submatrix size to all processes
	MPI_Bcast(&submatrixsize, 1, MPI_INT, 0, MPI_COMM_WORLD);

	//use mpi scatter to scatter data from process 0 to remaining processes
	int *submatrix = (int *) malloc(sizeof(int) * submatrixsize * submatrixsize);
	MPI_Scatter(data, submatrixsize*submatrixsize, MPI_INT, submatrix, submatrixsize*submatrixsize, MPI_INT, 0, MPI_COMM_WORLD);
		
	// Change communicator to 2-d grid
	int dimSize[2] = {0, 0};
	MPI_Dims_create(size, 2, dimSize);
	int periodic[2] = {0, 0};
	int myCoords[2];
	MPI_Comm comm2D;
	MPI_Cart_create(MPI_COMM_WORLD, 2, dimSize, periodic, 0, &comm2D);
	
	// get my processors coordinates in the grid
	int myRow, myCol;
	MPI_Cart_coords(comm2D, rank, 2, myCoords);
	myRow = myCoords[0]; myCol = myCoords[1];
	
	// split comm2D into row and column communicators
	MPI_Comm commR, commC;
	MPI_Comm_split(comm2D, myRow, myCol, &commR);
	MPI_Comm_split(comm2D, myCol, myRow, &commC);
	
	
	// do prefix sum column wise on local submatrix - CAN DO IN PARALLEL FOR FASTER EXECUTION using openmp
	for (int i = 1; i < submatrixsize; i++) {
		for (int j = 0; j < submatrixsize; j++) {
			submatrix[i*submatrixsize + j] += submatrix[(i-1)*submatrixsize + j];
		}
	}
	
	// prefix sum across column processes
	int *previouscolumnsum = (int *) malloc(sizeof(int) * submatrixsize);
	MPI_Exscan(&submatrix[(submatrixsize-1)*submatrixsize], previouscolumnsum, submatrixsize, MPI_INT, MPI_SUM, commC);
	
	// add previous column sum to local submatrix
	if (myRow > 0) {
		for (int i = 0; i < submatrixsize; i++) {
			for (int j = 0; j < submatrixsize; j++) {
				submatrix[i*submatrixsize + j] += previouscolumnsum[j];
			}
		}
	}
	
	// do prefix sum row wise on localsubmatrix - CAN DO IN PARALLEL FOR FASTER EXECUTION using openmp
	for (int i = 0; i < submatrixsize; i++) {
		for (int j = 1; j < submatrixsize; j++) {
			submatrix[i*submatrixsize + j] += submatrix[i*submatrixsize + (j-1)];
		}
	}
	
	// prefix sum across row processes
	int *mylocalrowsum = (int *) malloc(sizeof(int) * submatrixsize);
	int *previousrowsum = (int *) malloc(sizeof(int) * submatrixsize);
	for (int i = 0; i < submatrixsize; i++) {
		mylocalrowsum[i] = submatrix[i * submatrixsize + (submatrixsize-1)];
	}
	MPI_Exscan(mylocalrowsum, previousrowsum, submatrixsize, MPI_INT, MPI_SUM, commR);
	
	// add previous column sum to local submatrix
	if (myCol > 0) {
		for (int i = 0; i < submatrixsize; i++) {
			for (int j = 0; j < submatrixsize; j++) {
				submatrix[i*submatrixsize + j] += previousrowsum[i];
			}
		}
	}
	
	//free malloc'ed memory
	free(submatrix);
	free(previouscolumnsum);
	free(mylocalrowsum);
	free(previousrowsum);
	
	//Finalize
	MPI_Finalize();
	return 0;
}











