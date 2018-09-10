#include <stdio.h>
#include "mpi.h"

int main(int argc, char** argv) {


    MPI_Init(&argc, &argv);

    int myRank;
    int totalProcesses;
    int tag = 50;
    MPI_Status status;
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    MPI_Comm_size(MPI_COMM_WORLD, &totalProcesses);

    int N = 10;

    int message;
    int vetor[10] = {0, 1,2,3,4,5,6,7,8,9};

    for (int i = 0; i< N; i++) {
        //recebe
        if (myRank == 0) {
            message = vetor[i];
        }
        else {
            MPI_Recv(&message, 1, MPI_INT, myRank - 1, tag, MPI_COMM_WORLD, &status);
        }

      //  printf("Process %i processing value...\n", myRank);

        //processa
        message += 1;

        //envia
        if (myRank == totalProcesses - 1) {
            printf("Last process %i received %i\n", myRank, message);
        } else {
            MPI_Send(&message, 1, MPI_INT, myRank + 1, tag, MPI_COMM_WORLD);
        }
    }
    MPI_Finalize();
}