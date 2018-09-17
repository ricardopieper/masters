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

    int msg[10];
    
    int M[10][10];

    if (myRank == 0) {
        for (int l=0; l<N; l++) 
        for (int c=0; c<N; c++)
            M[l][c] = 0;
    }
    
    for (int i = 0; i< N; i++) {
        //recebe
        if (myRank == 0) {
            for (int c=0; c<N; c++) 
                msg[c] = M[i][c];
        }
        else {
            MPI_Recv(msg, 10, MPI_INT, myRank - 1, tag, MPI_COMM_WORLD, &status);
        }

      //  printf("Process %i processing value...\n", myRank);

        //processa 
        for (int c=0; c<N; c++) 
            msg[c] = msg[c] + 1;

        //envia
        if (myRank == totalProcesses - 1) {
            printf("received [");
            for (int c = 0; c < N; c++)
                printf("%i ", msg[c]);
            printf("]\n");  
        } else {
            MPI_Send(msg, 10, MPI_INT, myRank + 1, tag, MPI_COMM_WORLD);
        }
    }
    MPI_Finalize();
}