
#include <mutex>
#include <deque>
#include <condition_variable>
#include <vector>
#include <iostream>
#include <thread>
#include <map>

template<typename T>
class queue {
private:
    std::mutex d_mutex;
    std::condition_variable d_condition;
    std::deque<T> d_queue;
public:
    void push(T const &value) {
        {
            std::unique_lock<std::mutex> lock(this->d_mutex);
            d_queue.push_front(value);
        }
        this->d_condition.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(this->d_mutex);
        this->d_condition.wait(lock, [=] { return !this->d_queue.empty(); });
        T rc(std::move(this->d_queue.back()));
        this->d_queue.pop_back();
        return rc;
    }
};


typedef struct StageResult {
    int tag;
    void *data;
} StageResult;


class Stage {
public:

    explicit Stage(int threads) {
        this->threads = threads;
    };

    virtual void *process(void *task) = 0;

    int threads;

    std::vector<std::thread> workerThreads;

    queue<StageResult *> workQueue;
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
        //inicia threads para cada um deles
        //cada thread dá um .pop na fila
        //e quando popar um NULL encerra

        //stages vão de 0 a N-1
        //para as stages 1..N-1, as threads vai ficar fazendo pop até ver um null

        for (int stageId = 1; stageId < stages.size(); stageId++) {

            Stage *stage = stages[stageId];
            Stage *prevStage = stages[stageId - 1];
            Stage *nextStage = nullptr;

            if (stageId < stages.size() - 1) {
                nextStage = stages[stageId + 1];
            }

            for (int i = 0; i < stage->threads; i++) {

                std::thread thread = std::thread([stage, nextStage, this]() {
                    StageResult *result;
                    while ((result = stage->workQueue.pop()) != nullptr) {

                        void *nextResult = stage->process(result->data);

                        if (nextStage) {
                            if (nextResult != nullptr) {
                                auto *nextStageResult = new StageResult();
                                nextStageResult->data = nextResult;
                                nextStageResult->tag = result->tag;
                                nextStage->workQueue.push(nextStageResult);
                            } else {
                                nextStage->workQueue.push(nullptr);
                            }
                        }

                        delete result;
                    }

                });

                stage->workerThreads.push_back(std::move(thread));
            }

        }

        //para o stage 0, ela mesma fica gerando valores até ela retornar null
        Stage* firstStage = stages[0];
        Stage* secondStage = stages[1];
        firstStage->workerThreads.emplace_back([firstStage, secondStage]{
            int tag = 0;

            void* result;

            while ((result = firstStage->process(nullptr)) != nullptr) {
                auto * stageResult = new StageResult();
                stageResult->data = result;
                stageResult->tag = tag++;
                secondStage->workQueue.push(stageResult);
            }

            secondStage->workQueue.push(nullptr);
        });

        //join em todas as threads

        for (Stage* stage: stages) {
            for (std::thread& th : stage->workerThreads) {
                th.join();
            }
        }

    }
};

class GenerateWork : public Stage {
public:
    GenerateWork() : Stage(1) {}
    int state = 0;
    void *process(void *in_task) override {
        if (state > 100) {
            return nullptr;
        }
        state++;
        int* newState = new int;
        *newState = state;
        std::cout<<"newstate = "<<(*newState)<<std::endl;
        return newState;
    }
};


class Process : public Stage {
public:
    Process() : Stage(1) {}

    void *process(void *in_task) override {
        int* state = (int*)in_task;
        *state *= 2;
        return in_task;
    }
};


class Collect : public Stage {
public:
    Collect() : Stage(1) {}
    int sum = 0;
    void *process(void *in_task) override {
        int* state = (int*)in_task;
        sum += *state;
        std::cout<<"sum so far: "<<(sum)<<std::endl;
        return nullptr;
    }
};

