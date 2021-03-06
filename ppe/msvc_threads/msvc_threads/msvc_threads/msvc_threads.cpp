// msvc_threads.cpp : Este arquivo contém a função 'main'. A execução do programa começa e termina ali.
//

#include "pch.h"
#include <iostream>
#include <mutex>
#include <deque>
#include <vector>


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

	void clear() {
		this->d_queue.clear();
	}

	unsigned int size() {
		return this->d_queue.size();
	}

	std::deque<T> raw_queue() {
		return this->d_queue;
	}
};

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

					while (true) {
						result = stage->workQueue.pop();

						if (result != nullptr) {
							void *nextResult = stage->process(result->data);
							if (nextStage != nullptr) { //last stage does not have a next stage
								if (nextResult != nullptr) {
									auto *nextStageResult = new StageResult();
									nextStageResult->data = nextResult;
									nextStageResult->tag = result->tag; //propagate the tag
									nextStage->workQueue.push(nextStageResult);
								}
								else {
									std::cout << "THIS SHOULD NEVER HAPPEN, REVIEW STAGE CONTRACT" << std::endl;
								}
							}
							delete result;
						}
						else {
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

		for (Stage *stage : stages) {
			//when all threads have finished, this means that
			//there may be a bunch of nullptr values in the queue, since every thread that finishes
			//also add a finish signal so other threads can pick it up
#ifdef DEBUG
			std::cout << "Number of finish signals for stage " << stage->name << ": " << stage->workQueue.size() << std::endl;
			for (StageResult *garbage : stage->workQueue.raw_queue()) {
				if (garbage == nullptr) {
					std::cout << stage->name << " -> NULLPTR" << std::endl;
				}
				else {
					std::cout << stage->name << " " << garbage->tag << std::endl;
				}
			}
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
		if (state == 10000) {
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
	Process() : Stage(8, "Process") {}

	void *process(void *in_task) override {
		int *state = (int *)in_task;
		*state *= 1;
		return in_task;
	}
};


class Collect : public Stage {
public:
	Collect(int *sum) : Stage(1, "Collect") {
		this->sum = sum;
	}

	int *sum;

	void *process(void *in_task) override {
		int *state = (int *)in_task;
		*sum += *state;
		return nullptr;
	}
};


int main() {

	for (int i=0; i< 1000; i++) {
		Pipeline p = Pipeline();
		int sum = 0;
		GenerateWork gw;
		Process proc;
		Collect collect(&sum);

		p.add(&gw);
		p.add(&proc);
		p.add(&collect);

		p.start();
		std::cout << "pipeline is finished, result: " << sum << std::endl;

	}

}