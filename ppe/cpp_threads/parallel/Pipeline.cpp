
#include <mutex>
#include <deque>
#include <condition_variable>
#include <vector>
#include <iostream>
#include <thread>
#include <map>
#include "dalvan_queue.cpp"

#define DEBUG2

typedef struct StageResult {
    int tag;
    void *data;
} StageResult;


class Stage {
public:

    explicit Stage(int threads, std::string name) {
        this->threads = threads;
        this->name = name;
    };

    virtual void *process(void *task) = 0;

    int threads;
    std::string name;

    std::vector<std::thread> workerThreads;

    comm::queue<StageResult *> workQueue;
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

            for (int i = 0; i < stage->threads; i++) {

                std::thread thread = std::thread([stage, nextStage, this]() {
                    StageResult *result;
                    while ((result = stage->workQueue.pop()) != nullptr) {
                        void *nextResult = stage->process(result->data);

                        if (nextStage != nullptr) { //last stage does not have a next stage
                            if (nextResult != nullptr) {
                                auto *nextStageResult = new StageResult();
                                nextStageResult->data = nextResult;
                                nextStageResult->tag = result->tag;
#ifdef DEBUG
                                std::cout << "|| Stage " << stage->name << " pushing into Stage " << nextStage->name
                                          << " tag " << (nextStageResult->tag) << std::endl;
#endif
                                nextStage->workQueue.push(nextStageResult);
                            } else {
#ifdef DEBUG
                                std::cout << "|| Stage " << stage->name << " pushing into Stage " << nextStage->name
                                          << " NULLPTR" << std::endl;
#endif
                                nextStage->workQueue.push(nullptr);
                                stage->workQueue.push(nullptr);
                                break;
                            }
                        }

                        delete result;
                    }

                    if (nextStage != nullptr) {

                //        std::cout << "|| Stage " << stage->name
                 //                 << " popped NULLPTR and is finishing. Also forcing next stage to finish" << std::endl;
                        nextStage->workQueue.push(nullptr);
                    } else {

                    //    std::cout << "|| Stage " << stage->name
                      //            << " popped NULLPTR and is finishing. It's the last stage in the pipeline, so there's no next stage to force finishing"
                       //           << std::endl;
                    }

                    std::cout << "||FT->" << stage->name<< std::endl;
                });

                stage->workerThreads.push_back(std::move(thread));
            }

        }

        //for stage 0, we only create 1 thread. Keep making it generate work until it returns a nullptr
        Stage *firstStage = stages[0];
        Stage *secondStage = stages[1];
        firstStage->workerThreads.emplace_back([firstStage, secondStage] {
            int tag = 0;
            void *result;
            while ((result = firstStage->process(nullptr)) != nullptr) {
                auto *stageResult = new StageResult();
                stageResult->data = result;
                stageResult->tag = tag++;
#ifdef DEBUG
                std::cout << "|| Stage " << firstStage->name << " pushing into Stage " << secondStage->name << " tag "
                          << (stageResult->tag) << std::endl;
#endif
                secondStage->workQueue.push(stageResult);
            }
#ifdef DEBUG
            std::cout << "|| Stage " << firstStage->name << " pushing into Stage " << secondStage->name << " NULLPTR"
                      << std::endl;
#endif
            secondStage->workQueue.push(nullptr);
        });

        //join all threads

        for (Stage *stage: stages) {
            for (std::thread &th : stage->workerThreads) {

   //             std::cout << "Joining thread for stage " << stage->name << std::endl;
                th.join();
                std::cout << "J " << stage->name << std::endl;
            }
        }

        for (Stage *stage: stages) {
            //when all threads have finished, this means that
            //there may be a bunch of nullptr values in the queue, since every thread that finishes
            //also add a finish signal so other threads can pick it up
#ifdef DEBUG
            std::cout << "Number of finish signals for stage "<<stage->name<<": " <<stage->workQueue.size()<<std::endl;
#endif
            stage->workQueue.clear();
        }

    }
};

class GenerateWork : public Stage {
public:
    GenerateWork() : Stage(1, "GenerateWork") {}

    int state = 0;

    void *process(void *in_task) override {
        if (state == 10) {
            return nullptr;
        }
        state++;
        int *newState = new int;
        *newState = state;
        return newState;
    }
};


class Process : public Stage {
public:
    Process() : Stage(200, "Process") {}

    void *process(void *in_task) override {
        int *state = (int *) in_task;
        *state *= 1;
        return in_task;
    }
};


class Collect : public Stage {
public:
    Collect(int *sum) : Stage(1, "Collect") {
        this->sum = sum;
        std::cout << "Starting state " << (*sum) << std::endl;
    }

    int *sum;

    void *process(void *in_task) override {
        int *state = (int *) in_task;
       // std::cout << "Popped " << (*state) << std::endl;
        *sum += *state;
        //std::cout << "Collected "<<(*sum)<<std::endl;
        return nullptr;
    }
};

