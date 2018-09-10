#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

int main(int argc, char** argv) {


    MPI_Init(&argc, &argv);

    int myRank;
    int totalProcesses;
    int tag = 50;
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    MPI_Comm_size(MPI_COMM_WORLD, &totalProcesses);
    
    int M = 10;
    int N = 10;
    MPI_Status status;
    int* array = malloc(N * sizeof(int));
    //producer
    if (myRank == 0) {

        int** matrix;
        matrix = malloc(M * sizeof(int));
        for (int i=0; i< M; i++) {
            matrix[i] = malloc(N * sizeof(int)); 
            for (int j=0; j< M; j++) {
                matrix[i][j] = i * j;
            }
        }

        printf("Process %i starting pipeline...\n", myRank);
        
        for (int i=0; i< M; i++) {
            MPI_Send(matrix[i], N, MPI_INT, myRank + 1, tag, MPI_COMM_WORLD);
        }

    } else {

        int receiveFrom = myRank - 1;
        MPI_Recv(&array, N, MPI_INT, receiveFrom, tag, MPI_COMM_WORLD, &status);
    }
    
    
    //processa msg

    if (myRank == totalProcesses - 1) //se for o ultimo
    {
        printf("Ultimo");
    }
    
    
     else { //consumer-producer
        int receiveFrom = myRank - 1;
        int sendTo = myRank + 1;
       
        for (int i=0; i< M; i++) {
            int* array = malloc(N * sizeof(int));
            printf("Process %i receiving from %i and passing value to %i ...\n", myRank, receiveFrom, sendTo);
            MPI_Recv(&array, N, MPI_INT, receiveFrom, tag, MPI_COMM_WORLD, &status);
            MPI_Send(&array, N, MPI_INT, sendTo, tag, MPI_COMM_WORLD);
        }
  
    } else if (myRank == totalProcesses - 1) { //consumer
        int receiveFrom = myRank - 1;

        for (int i=0; i< M; i++) {
            int* array = malloc(N * sizeof(int));
            MPI_Recv(&array, N, MPI_INT, receiveFrom, tag, MPI_COMM_WORLD, &status);

            for (int j=0; j< N; j++) {
               printf("Value at [%i][%i] = [%i]\n", i, j, array[j]);
            }

        }

        printf("FINISHED!");
    }

    MPI_Finalize();
}