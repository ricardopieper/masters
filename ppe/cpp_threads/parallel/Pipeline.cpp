#include <utility>


#include <mutex>
#include <deque>
#include <condition_variable>
#include <vector>
#include <iostream>
#include <thread>
#include <map>
#include <string>
#include <set>
#include "dalvan_queue.cpp"

#define DEBUG2

typedef struct StageResult {
    int tag;
    void *data;

    bool operator()(const StageResult *lhs, const StageResult *rhs) const {
        return lhs->tag < rhs->tag;
    }
} StageResult;


class Stage {
public:

    explicit Stage(int threads, bool ordered = false, std::string name = "Stage") {
        this->threads = threads;
        this->name = std::move(name);
        this->ordered = ordered;
    };

    virtual void *process(void *task) = 0;

    int threads;
    bool ordered;
    std::string name;
    std::vector<std::thread> workerThreads;
    comm::queue<StageResult *> workQueue;
    std::set<StageResult *, StageResult> orderedWorkQueue;
    int orderedTag;
};


class Pipeline {
private:
    std::vector<Stage *> stages;
public:

    Pipeline() {
        stages = std::vector<Stage *>();
    }

    void add(Stage *stage) {
        stages.push_back(stage);
    };

    void start() {
        //start threads for all stages
        //each stage pops its work queue
        //if the pop result == nullptr then finish thread

        //stages go from 0 to n-1, where 0 is kind of special
        //for stages from 1 to n-1, threads will keep popping until a nullptr appears

        for (int stageId = 1; stageId < stages.size(); stageId++) {

            Stage *stage = stages[stageId];
            Stage *nextStage = nullptr;

            if (stageId < stages.size() - 1) {
                nextStage = stages[stageId + 1];
            }

            //if we are ordered, then allow only 1 thread to be created
            int threads = stage->ordered ? 1 : stage->threads;

            for (int i = 0; i < threads; i++) {

                std::thread thread = std::thread([threads, stage, nextStage, this]() {

                    if (!stage->ordered) {
                        while (true) {
                            StageResult *result = stage->workQueue.pop();

                            if (result != nullptr) {
                                runStage(stage, nextStage, result);
                                delete result;
                            } else {
                                //if we receive a nullptr, this means that all of our work has already been sent to
                                //the next stage, so we don't need to worry anymore
                                //and also notify another thread that the work is done, so other threads may finish
                                stage->workQueue.push(nullptr);
                                //we can expect that in the end we'll have a single NULLPTR value in the
                                //work queue still remaining to be consumed. This is expected because
                                //the last thread will try to notify any alive threads that the work is done.
                                //no one will ever listen, but this is normal.
                                break;
                            }
                        }
                    } else {

                        while (true) {
                            StageResult *poppedResult = stage->workQueue.pop();
                            if (poppedResult == nullptr) break;

                            if (poppedResult->tag == stage->orderedTag) {

                                runStage(stage, nextStage, poppedResult);
                                stage->orderedTag++;
                                checkOrderedWorkQueue(stage, nextStage);

                            } else {
                                stage->orderedWorkQueue.insert(poppedResult);
                                checkOrderedWorkQueue(stage, nextStage);
                            }

                        }
                    }


                });

                stage->workerThreads.push_back(std::move(thread));
            }

        }

        Stage *firstStage = stages[0];
        Stage *secondStage = stages[1];
        int tag = 0;
        void *result;

        while ((result = firstStage->process(nullptr)) != nullptr) {
            auto *stageResult = new StageResult();
            stageResult->data = result;
            stageResult->tag = tag++;
            secondStage->workQueue.push(stageResult);
        }
        secondStage->workQueue.push(nullptr);

        //join all threads from multithreaded stages
        for (int i = 1; i < stages.size(); i++) {
            Stage *stage = stages[i];
            Stage *nextStage = nullptr;

            if (i < stages.size() - 1) {
                nextStage = stages[i + 1];
            }
            for (std::thread &th : stage->workerThreads) {
                //             std::cout << "Joining thread for stage " << stage->name << std::endl;
                th.join();
                //std::cout << "J " << stage->name << std::endl;
            }
            //when all threads of this stage have finished, send a nullptr to the next stage
            if (nextStage != nullptr) {
                nextStage->workQueue.push(nullptr);
            }
            //the last stage will receive a single nullptr only when all the threads from the previous stage have
            //finished
        }

        for (Stage *stage: stages) {
            //when all threads have finished, this means that
            //there may be a bunch of nullptr values in the queue, since every thread that finishes
            //also add a finish signal so other threads can pick it up
#ifdef DEBUG
            std::cout << "Number of finish signals for stage "<<stage->name<<": " <<stage->workQueue.size()<<std::endl;
            for (StageResult *garbage : stage->workQueue.raw_queue()) {
                if (garbage == nullptr) {
                    std::cout << stage->name << " -> NULLPTR" << std::endl;
                } else {
                    std::cout << stage->name << " " << garbage->tag << std::endl;
                }
            }
#endif
            stage->workQueue.clear();
        }

    }

    void checkOrderedWorkQueue(Stage *stage, Stage *nextStage) const {
        StageResult *poppedResult = nullptr;
        while (true) {

            auto it = stage->orderedWorkQueue.begin();
            auto it_end = stage->orderedWorkQueue.end();
            if (it == it_end) {
                break;
            } else {

                poppedResult = *it;

                if (poppedResult->tag == stage->orderedTag) {
                    runStage(stage, nextStage, poppedResult);
                    stage->orderedWorkQueue.erase(it);
                    stage->orderedTag++;
                } else {
                    break;
                }
            }
        }
    }

    void runStage(Stage *stage, Stage *nextStage, StageResult *result) const {
        void *nextResult = stage->process(result->data);

        if (nextStage != nullptr) { //last stage does not have a next stage
            if (nextResult != nullptr) {
                auto *nextStageResult = new StageResult();
                nextStageResult->data = nextResult;
                nextStageResult->tag = result->tag; //propagate the tag
                nextStage->workQueue.push(nextStageResult);
            } else {
                std::cout << "REVIEW IF YOU ADDED THE STAGES IN THE INTENDED ORDER,"
                             " SOMETHING IS DEFINITELY WRONG" << std::endl;
            }
        }
    }
};


