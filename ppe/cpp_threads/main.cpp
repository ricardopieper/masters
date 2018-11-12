#include <iostream>
#include "parallel/Pipeline.cpp"

class GenerateWork : public Stage {
public:
	GenerateWork() : Stage(1, "GenerateWork") {}

	int state = 0;

	void *process(void *in_task) override {
		if (state == 100000) {
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
		//if (*state % 1000 == 0) {
		//	std::cout << "CP " << (*state) << std::endl;
		//}
		*sum += *state;
		delete in_task;
		return nullptr;
	}
};


int main() {

    for (int i=0; i< 100; i++) {
        Pipeline p = Pipeline();
        int sum = 0;
        GenerateWork gw;
        Process proc;
        Collect collect(&sum);

        p.add(&gw);
        p.add(&proc);
        p.add(&collect);

        p.start();

        std::cout<<"pipeline "<<i <<" is finished, result: "<<sum<<std::endl;
    }
}