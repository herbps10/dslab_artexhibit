#include "mpi.h"
#include <stdio.h>

int main(int argv, char **argc)
{
	MPI_Init(&argv, &argc);
	
	int rank, size;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	printf("rank %i of %i\n", rank, size);
	
	MPI_Finalize();
}
