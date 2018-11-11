//
// Created by ricardo on 10/10/18.
//

#ifndef CPP_THREADS_STATICPARALLELFOR_H
#define CPP_THREADS_STATICPARALLELFOR_H


#include <functional>
#include <thread>
#include <iostream>


class Range {
public:
    int from;
    int to;
};

class StaticParallelFor {
private:
    int from;
    int to;
    int workers;
public:
    StaticParallelFor(int from, int to, int workers) :
            from(from), to(to), workers(workers) {

    }


    void run(const std::function<void(const Range &r)> &function) {

        //o ultimo worker vai receber o modulo da divisao
        int workSize = to - from;
        int tasksPerWorker = workSize / workers;
        int remainderWork = workSize % workers;


        auto *ranges = new Range[workers];


        //para todos exceto o ultimo
        for (int i = 0; i < workers - 1; i++) {
            Range r = Range();
            r.from = tasksPerWorker * i;
            r.to = tasksPerWorker * (i + 1);

            ranges[i] = r;
        }
        Range lastRange = Range();
        lastRange.from = tasksPerWorker * (workers - 1);
        lastRange.to = tasksPerWorker * (workers) + remainderWork;

        ranges[workers - 1] = lastRange;


        std::thread workerThreads[workers];

        for (int i = 0; i < workers; i++) {
            workerThreads[i] = std::thread(function, ranges[i]);
        }

        for (auto &t : workerThreads) {
            t.join();
        }

        delete[] ranges;
    }

};


#endif //CPP_THREADS_STATICPARALLELFOR_H
