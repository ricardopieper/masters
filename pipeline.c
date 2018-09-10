#include <stdio.h>
#include "mpi.h"

int main(int argc, char** argv) {


    MPI_Init(&argc, &argv);

    int myRank;
    int totalProcesses;
    int tag = 50;
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    MPI_Comm_size(MPI_COMM_WORLD, &totalProcesses);

    //producer
    if (myRank == 0) {
        printf("Process %i producing value...\n", myRank);
        int dataToSend = 500;
        MPI_Send(&dataToSend, 1, MPI_INT, myRank + 1, tag, MPI_COMM_WORLD);
    } else if (myRank < (totalProcesses - 1)) { //consumer-producer
        int received;
        int receiveFrom = myRank - 1;
        int sendTo = myRank + 1;
        MPI_Status status;
        MPI_Recv(&received, 1, MPI_INT, receiveFrom, tag, MPI_COMM_WORLD, &status);
        printf("Process %i receiving from %i and passing value to %i ...\n", myRank, receiveFrom, sendTo);
        MPI_Send(&received, 1, MPI_INT, sendTo, tag, MPI_COMM_WORLD);
    } else if (myRank == totalProcesses - 1) { //consumer
        int received;
        int receiveFrom = myRank - 1;
        MPI_Status status;
        MPI_Recv(&received, 1, MPI_INT, receiveFrom, tag, MPI_COMM_WORLD, &status);
        printf("FINISHED! Process %i received from %i from %i ...\n", myRank, received, receiveFrom);
    }

    MPI_Finalize();
}