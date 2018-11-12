#include <iostream>
#include "parallel/Pipeline.cpp"

#include <stdio.h>
#include "marX2.h"
#include <sys/time.h>
#include <math.h>


#define DIM 800
#define ITERATION 1024

struct LineToCalculate {
    int line;
};

struct LineToRender {
    char* M;
    int line;
};


class GenerateWork : public Stage {
private:
    int numberOfLines;
    int currentNumber;
    int dimensions;
public:
    GenerateWork(int numberOfLines) : Stage(1, false, "GenerateWork") {
        this->numberOfLines = numberOfLines;
        this->currentNumber = 0;
        this->dimensions = numberOfLines;
    }
    void *process(void *in_task) override {
        if (currentNumber < numberOfLines) {
            auto lineToCalculate = new LineToCalculate { .line = currentNumber };
            currentNumber++;
            return lineToCalculate;
        } else {
            return nullptr;
        }
    }
};


class CalculateLine : public Stage {
private:
    int dimensions;
    double init_a;
    double init_b;
    double step;
    int niter;
public:
    CalculateLine(
            int dimensions, double init_a,
            double init_b, double step,
            int niter, int threads) : Stage(threads, false, "CalculateLine") {
        this->dimensions = dimensions;
        this->init_b = init_b;
        this->init_a = init_a;
        this->step = step;
        this->niter = niter;
    }
    void *process(void *in_task) override {
        int dim = dimensions;
        auto lineToCalculate = static_cast<LineToCalculate*>(in_task);
        char* M = new char[dimensions];
        int i = lineToCalculate->line;

        double im;
        im = init_b + (step * i);
        for (int j = 0; j < dim; j++) {
            double cr;
            double a = cr = init_a + step * j;
            double b = im;
            int k = 0;
            for (; k < niter; k++) {
                double a2 = a * a;
                double b2 = b * b;
                if ((a2 + b2) > 4.0) break;
                b = 2 * a * b + im;
                a = a2 - b2 + cr;
            }
            M[j] = (unsigned char) 255 - ((k * 255 / niter));
        }
        auto lineToRender = new LineToRender { .M = M, .line = lineToCalculate->line };
        delete lineToCalculate;
        return lineToRender;
    }
};

class RenderLine : public Stage {
private:
    int dimensions;
public:
    RenderLine(int dimensions) : Stage(1, true, "RenderLine") {
        this->dimensions = dimensions;
    }
    void * process(void *in_task) {

        auto lineToRender = static_cast<LineToRender*>(in_task);

#if !defined(NO_DISPLAY)
        ShowLine(lineToRender->M, this->dimensions, lineToRender->line);
#endif
        delete[] lineToRender->M;
        delete lineToRender;
        return nullptr;
    }
};


double diffmsec(struct timeval a, struct timeval b) {
    long sec = (a.tv_sec - b.tv_sec);
    long usec = (a.tv_usec - b.tv_usec);

    if (usec < 0) {
        --sec;
        usec += 1000000;
    }
    return ((double) (sec * 1000) + (double) usec / 1000.0);
}



int main(int argc, char **argv) {
    XInitThreads();
    std::cout << "#pipeline(seq,farm(seq),seq) pattern implementation!!" << std::endl;
    double init_a = -2.125, init_b = -1.5, range = 3.0;
    int j, i, k;
    double step, im, a, b, a2, b2, cr;
    int dim = DIM, niter = ITERATION;
    // stats
    struct timeval t1, t2;
    int r, retries = 1, threads=4;
    double avg = 0, var, *runs;


    if (argc < 3) {
        printf("Usage: pipeline_farm size niterations retries threads\n\n\n");
    } else {
        dim = atoi(argv[1]);
        niter = atoi(argv[2]);
        step = range / ((double) dim);
        retries = atoi(argv[3]);
        threads = atoi(argv[4]);
    }
    runs = (double *) malloc(retries * sizeof(double));


    printf("Mandebroot set from (%g+I %g) to (%g+I %g)\n",
           init_a, init_b, init_a + range, init_b + range);
    printf("resolution %d pixel, Max. n. of iterations %d\n", dim * dim, niter);

    step = range / ((double) dim);


#if !defined(NO_DISPLAY)
    SetupXWindows(dim, dim, 1, NULL, "Pipeline(Seq,farm(seq),seq) Mandelbrot");
#endif

    for (r = 0; r < retries; r++) {

        // Start time
        gettimeofday(&t1, NULL);

        Pipeline p;
        GenerateWork gw(dim);
        CalculateLine cl(dim, init_a, init_b, step, niter, threads);
        RenderLine rl(dim);
        p.add(&gw);
        p.add(&cl);
        p.add(&rl);

        p.start();
        // Stop time
        gettimeofday(&t2, NULL);

        avg += runs[r] = diffmsec(t2, t1);
        printf("Run [%d] DONE, time= %f (ms)\n", r, runs[r]);
    }
    avg = avg / (double) retries;
    var = 0;
    for (r = 0; r < retries; r++) {
        var += (runs[r] - avg) * (runs[r] - avg);
    }
    var /= retries;
    printf("Average on %d experiments = %f (ms) Std. Dev. %f\n\nPress a key\n", retries, avg, sqrt(var));

#if !defined(NO_DISPLAY)
    CloseXWindows();
#endif

    return 0;
}
