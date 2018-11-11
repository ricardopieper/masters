//
// Created by ricardo on 24/10/18.
//

#include <functional>
#include <thread>
#include "dalvan_queue.cpp"


class DynamicRange {
public:
    int from;
    int to;
};

class DynamicParallelFor {
private:
    int from;
    int to;
    int workers;
    int granularity;
    comm::queue<DynamicRange> queue;
public:
    DynamicParallelFor(int from, int to, int granularity, int workers) :
            from(from), to(to), granularity(granularity), workers(workers) {
        //o ultimo worker vai receber o modulo da divisao
        int workSize = to - from;
        int numberOfGrains = workSize / granularity;
        int remainderWork = workSize % granularity;


        for (int i = 0; i < numberOfGrains; i++) {

            DynamicRange r = DynamicRange();
            r.from = granularity * i;
            r.to = granularity * (i + 1);
            queue.push(r);
        }

        DynamicRange lastRange = DynamicRange();
        lastRange.from = granularity * numberOfGrains;
        lastRange.to = (granularity * numberOfGrains) + remainderWork;

        queue.push(lastRange);
    }


    void run(const std::function<void(const DynamicRange &r)> &function) {

        std::thread workerThreads[workers];

        for (int i = 0; i < workers; i++) {
            workerThreads[i] = std::thread([this, function]() {

                PopResult<DynamicRange> popResult;
                while (!(popResult = queue.pop2()).empty) {
                    function(popResult.result);
                }

            });
        }

        for (auto &t : workerThreads) {
            t.join();
        }

    }

};
