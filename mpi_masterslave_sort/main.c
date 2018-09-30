#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"


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
        for (int j = i + 1; j < size; j++) {
            int val_j = array[j];
            if (val_i > val_j) {
                printf("found larger at %i %i (%i %i)\n", i, j, val_i, val_j);
                return 0;
            }
        }
    }

    return 1;
}

void heavyTesting(int tests, int sizeOfArrays) {

    for (int t = 0; t < tests; t++) {
        int *arr = malloc(sizeOfArrays * sizeof(int));

        for (int i = 0; i < sizeOfArrays; i++) {
            arr[i] = (int) random();
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


int main(int argc, char **argv) {

    heavyTesting(1000, 10000);

    return 0;
}