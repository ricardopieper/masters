#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include "mpi.h"

//The MPI usage of this program assumes MPI_Send and MPI_Recv are both blocking/synchronous.


//An in-place quicksort ad described by https://pt.wikipedia.org/wiki/Quicksort

int pivotIndexSelector(int *array, int lowerBound, int higherBound) {
    return (lowerBound + higherBound) / 2;
}

int qsPartition(int *array, int lowerBound, int higherBound) {
    //choose a pivot index given a choice policy
    int pivotIndex = pivotIndexSelector(array, lowerBound, higherBound);
    //get the value of the pivor
    int pivot = array[pivotIndex];

    //iterate over the current partition from both ends
    //so that I and J meet in the end
    int i = lowerBound;
    int j = higherBound;


    while (i <= j) {
        //find a number > pivot from the left
        while (array[i] < pivot) i++;
        //find a number < pivot from the right
        while (array[j] > pivot) j--;

        //do we need to swap them?
        //i<=j at this point means there are values to swap
        //when compared to the pivot
        if (i <= j) {
            //swap
            int tmp_i = array[i];
            array[i] = array[j];
            array[j] = tmp_i;
            //advance
            i++;
            j--;
        }
    }

    return i;
}

void doQuicksort(int *array, int lowerBound, int higherBound) {
    int sortedIndex = qsPartition(array, lowerBound, higherBound);

    if (lowerBound < sortedIndex - 1)
        doQuicksort(array, lowerBound, sortedIndex - 1);

    if (sortedIndex < higherBound)
        doQuicksort(array, sortedIndex, higherBound);
}


void quicksort(int *array, int size) {
    doQuicksort(array, 0, size - 1);
}

int isSorted(const int *array, int size) {

    for (int i = 0; i < size - 1; i++) {
        int val_i = array[i];
        int val_j = array[i + 1];
        if (val_i > val_j) {
            printf("found larger at %i %i (%i %i)\n", i, i + 1, val_i, val_j);
            return 0;
        }
    }

    return 1;
}

void heavyTesting(int tests, int sizeOfArrays) {

    for (int t = 0; t < tests; t++) {
        int *arr = malloc(sizeOfArrays * sizeof(int));

        for (int i = 0; i < sizeOfArrays; i++) {
            arr[i] = (int) rand();
        }

        quicksort(arr, sizeOfArrays);

        if (!isSorted(arr, sizeOfArrays)) {
            printf("array is not sorted...");

            for (int i = 0; i < sizeOfArrays; i++) {
                //      printf("%i\n", arr[i]);
            }
        }
    }

}

#define GET_TASK_MSGTAG 2
#define NO_MORE_TASKS_MSGTAG 3
#define RESULT_MSGTAG 4
#define TASK_MSGTAG 4
#define MASTER 0
#define ROWS 5000
#define COLS 50000

#define SHOW_RESULTS

typedef struct Task {
    int size;
    int taskNumber;
    int *unsortedArray;
} Task;

typedef struct Result {
    int size;
    int taskNumber;
    int *sortedArray;
} Result;


int *serializeTask(const Task *task, int *totalCount) {
    *totalCount = 2 + task->size;
    int *serialized = malloc(*totalCount * sizeof(int));
    serialized[0] = task->size;
    serialized[1] = task->taskNumber;
    for (int i = 2; i < COLS + 2; i++) {
        serialized[i] = task->unsortedArray[i - 2];
    }
    return serialized;
};

Task deserialize_task(int *taskData) {
    Task t;
    t.size = taskData[0];
    t.taskNumber = taskData[1];
    t.unsortedArray = &taskData[2];
    return t;
};

int *serialize_result(const Result *result, int *totalCount) {
    *totalCount = 2 + result->size;
    int *serialized = malloc(*totalCount * sizeof(int));
    serialized[0] = result->size;
    serialized[1] = result->taskNumber;
    for (int i = 2; i < COLS + 2; i++) {
        serialized[i] = result->sortedArray[i - 2];
    }
    return serialized;
};

Result deserialize_result(int *resultData) {
    Result r;
    r.size = resultData[0];
    r.taskNumber = resultData[1];
    r.sortedArray = &resultData[2];
    return r;
};

void printResult(Result r) {
    #ifdef SHOW_RESULTS
    printf("Checking whether task %i is sorted...", r.taskNumber);
    int sorted = isSorted(r.sortedArray, r.size);

    if (sorted) {
        printf("Task %i is sorted\n\n", r.taskNumber);
    } else {
        printf("Task %i is NOT sorted!!!!!!!!!!!!!!!!!\n\n", r.taskNumber);
    }
    #endif
}


void runMasterLoop(int numberOfProcesses) {

    int dummy = 0;
    MPI_Status status;
    int **M = malloc(ROWS * sizeof(int *));
    for (int r = 0; r < ROWS; r++) {
        M[r] = malloc(COLS * sizeof(int));
        for (int c = 0; c < COLS; c++) {
            M[r][c] = rand();
        }
    }

    int nextTask = 0;
    int completedTasks = 0;


    while (completedTasks < ROWS) {
        //Receive a message of any type
        printf("Master: waiting for slave request... \n");

        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        //after probing what kind of message we're getting, take appropriate action

        //if a slave is asking for work
        if (status.MPI_TAG == GET_TASK_MSGTAG) {
            //finish receiving message (throws message away)
            printf("Master: Slave %i wants work\n", status.MPI_SOURCE);
            MPI_Recv(&dummy, 1, MPI_INT, status.MPI_SOURCE, GET_TASK_MSGTAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (nextTask < ROWS) {
                //prepare his tasks
                Task t;
                t.size = COLS;
                t.taskNumber = nextTask;
                t.unsortedArray = M[t.taskNumber];
                int taskSize;
                int *buf = serializeTask(&t, &taskSize);

                MPI_Send(buf, taskSize, MPI_INT, status.MPI_SOURCE, TASK_MSGTAG, MPI_COMM_WORLD);
                printf("Master: Sent task to slave %i\n", status.MPI_SOURCE);
                //now that the send is done, free the buffer
                free(buf);
                ++nextTask;
            } //we have no more tasks for them... keep them waiting for a bit

        } else if (status.MPI_TAG == RESULT_MSGTAG) {
            //we received a result back from a slave. Print the result
            //and free the task buffer we allocated previously
            //first, get the result buffer size
            int bufLength;
            MPI_Get_count(&status, MPI_INT, &bufLength);
            //allocate only what's necessary
            int *recvBuffer = malloc(bufLength * sizeof(int));
            MPI_Recv(recvBuffer, bufLength, MPI_INT, status.MPI_SOURCE, RESULT_MSGTAG, MPI_COMM_WORLD, &status);

            //deserialize and print
            Result r = deserialize_result(recvBuffer);
            printResult(r);

            //do bookkeeping and cleaning
            completedTasks++;
            free(recvBuffer);
        }
    }

    //now that all tasks are complete, release slaves from their misery
    for (int i = 1; i < numberOfProcesses; i++) {
        printf("Master:Releasing slave %i\n", i);

        MPI_Send(&dummy, 1, MPI_INT, i, NO_MORE_TASKS_MSGTAG, MPI_COMM_WORLD);
    }
}

void runSlaveLoop(int myRank) {
    int dummy = 0;
    MPI_Status status;
    //work forever until master releases us
    while (1) {
        //ask for work
        MPI_Send(&dummy, 1, MPI_INT, MASTER, GET_TASK_MSGTAG, MPI_COMM_WORLD);
        printf("Slave: asking master for work.... \n");
        //master can either give a task or release us
        //figure out what master wants
        MPI_Probe(MASTER, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if (status.MPI_TAG == NO_MORE_TASKS_MSGTAG) {
            printf("Slave: master says there's no more work.... \n");
            break;
        } else if (status.MPI_TAG == TASK_MSGTAG) {
            //we have received a task
            int count;
            MPI_Get_count(&status, MPI_INT, &count);
            int *taskBuffer = malloc(count * sizeof(int));
            MPI_Recv(taskBuffer, count, MPI_INT, MASTER, TASK_MSGTAG, MPI_COMM_WORLD, &status);
            Task t = deserialize_task(taskBuffer);

            printf("Slave: Doing the quicksort for task...%i\n", t.size);
            //do the in-place sorting
            quicksort(t.unsortedArray, t.size);

            //communicate back result
            Result r;
            r.size = t.size;
            r.taskNumber = t.taskNumber;
            r.sortedArray = t.unsortedArray; //the unsorted array is now sorted
            int totalSize;
            int *resultBuffer = serialize_result(&r, &totalSize);
            printf("Slave: Sending results back\n");
            MPI_Send(resultBuffer, totalSize, MPI_INT, 0, RESULT_MSGTAG, MPI_COMM_WORLD);
            free(resultBuffer);
            free(taskBuffer);
        }
    }

}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int myRank;
    int totalProcesses;

    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    MPI_Comm_size(MPI_COMM_WORLD, &totalProcesses);

    if (myRank == 0) {
        printf("Starting as master.... \n");
        //master will only do task scheduling
        runMasterLoop(totalProcesses);
    } else {
        printf("Starting as slave.... \n");
        runSlaveLoop(myRank);
    }

    MPI_Finalize();
    return 0;
}