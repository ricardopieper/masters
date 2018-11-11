#include <iostream>
#include "parallel/StaticParallelFor.h"
#include "parallel/DynamicParallelFor.cpp"
#include "parallel/Pipeline.cpp"


void staticParallelFor(int MX, int threads) {


    StaticParallelFor spf(0, MX, threads);


    long **M = new long *[MX];
    long **A = new long *[MX];
    long **B = new long *[MX];


    for (int i = 0; i < MX; i++) {
        A[i] = new long[MX];
        B[i] = new long[MX];
        M[i] = new long[MX];

        for (int j = 0; j < MX; j++) {
            A[i][j] = i + j + 1;
            B[i][j] = i + j + 1;
        }
    }

    spf.run([&M, &A, &B, MX](const Range &r) {
        for (int i = r.from; i < r.to; i++) {
            for (long int j = 0; j < MX; j++) {
                for (long int k = 0; k < MX; k++) {
                    M[i][j] += (A[i][k] * B[k][j]);
                }
            }
        }
    });


}

void dynamicParallelFor(int MX, int threads) {

    DynamicParallelFor dpf(0, MX, 50, threads);

    long **M = new long *[MX];
    long **A = new long *[MX];
    long **B = new long *[MX];


    for (int i = 0; i < MX; i++) {
        A[i] = new long[MX];
        B[i] = new long[MX];
        M[i] = new long[MX];

        for (int j = 0; j < MX; j++) {
            A[i][j] = i + j + 1;
            B[i][j] = i + j + 1;
        }
    }

    dpf.run([&M, &A, &B, MX](const DynamicRange &r) {
        for (int i = r.from; i < r.to; i++) {
            for (long int j = 0; j < MX; j++) {
                for (long int k = 0; k < MX; k++) {
                    M[i][j] += (A[i][k] * B[k][j]);
                }
            }
        }
    });

}

/*int main(int argc, char **argv) {
    int MX = 4;

    int threads = atoi(argv[1]);

    for (int i=0; i< 100000; i++) {
        std::cout<<i<<std::endl;
        dynamicParallelFor(MX, threads);
    }
    return 0;
}*/

int main() {

    Pipeline p = Pipeline();

    GenerateWork gw;
    Process proc;
    Collect collect;

    p.add(&gw);
    p.add(&proc);
    p.add(&collect);

    p.start();

}