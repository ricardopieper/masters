/* ***************************************************************************
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  As a special exception, you may use this file as part of a free software
 *  library without restriction.  Specifically, if other files instantiate
 *  templates or use macros or inline functions from this file, or you compile
 *  this file and link it with other files to produce an executable, this
 *  file does not by itself cause the resulting executable to be covered by
 *  the GNU General Public License.  This exception does not however
 *  invalidate any other reasons why the executable file might be covered by
 *  the GNU General Public License.
 *
 ****************************************************************************
 */

/*

   Author: Marco Aldinucci.
   email:  aldinuc@di.unipi.it
   marco@pisa.quadrics.com
   date :  15/11/97

Modified by:

****************************************************************************
 *  Author: Dalvan Griebler <dalvangriebler@gmail.com>
 *
 *  Copyright: GNU General Public License
 *  Description: This program simply computes the mandelbroat set.
 *  File Name: mandel.cpp
 *  Version: 1.0 (25/05/2018)
 *  Compilation Command: make
 ****************************************************************************
*/


#include <stdio.h>
#include "marX2.h"
#include <sys/time.h>
#include <math.h>

#include <iostream>
#include <chrono>
#include "tbb/tbb.h"
#include "tbb/tbb_stddef.h"


#define DIM 800
#define ITERATION 1024

struct LineToCalculate {
    int line;
};

struct LineToRender {
    char* M;
    int line;
};


class GenerateWork : public tbb::filter {
private:
    int numberOfLines;
    int currentNumber;
    int dimensions;
public:
    GenerateWork(int numberOfLines) : tbb::filter(tbb::filter::serial) {
        this->numberOfLines = numberOfLines;
        this->currentNumber = 0;
        this->dimensions = numberOfLines;
    }
    void *operator()(void *in_task) {

        if (currentNumber < numberOfLines) {
            LineToCalculate* lineToCalculate = new LineToCalculate { .line = currentNumber };
            currentNumber++;
            return lineToCalculate;
        } else {
            return NULL;
        }
    }
    void reset() {
        this->currentNumber = 0;
    }
};

class CalculateLine : public tbb::filter {
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
        int niter) : tbb::filter(tbb::filter::serial) {
        this->dimensions = dimensions;
        this->init_b = init_b;
        this->init_a = init_a;
        this->step = step;
        this->niter = niter;
    }
    void *operator()(void *in_task) {
        int dim = dimensions;
        LineToCalculate* lineToCalculate = static_cast<LineToCalculate*>(in_task);
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
        LineToRender* lineToRender = new LineToRender { .M = M, .line = lineToCalculate->line };
        delete lineToCalculate;
        return lineToRender;
    }
};

class RenderLine : public tbb::filter {
private:
    int dimensions;
public:
    RenderLine(int dimensions) : tbb::filter(tbb::filter::serial) {
        this->dimensions = dimensions;
    }
    void *operator()(void *in_task) {

        LineToRender* lineToRender = static_cast<LineToRender*>(in_task);
         
#if !defined(NO_DISPLAY)
        ShowLine(lineToRender->M, this->dimensions, lineToRender->line);
#endif
        delete[] lineToRender->M;
        delete lineToRender;
        return NULL;
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
    std::cout << "#pipeline(seq,seq,seq) pattern implementation!!" << std::endl;
    std::cout << "TBB version:" << TBB_INTERFACE_VERSION << std::endl;
    double init_a = -2.125, init_b = -1.5, range = 3.0;
    int j, i, k;
    double step, im, a, b, a2, b2, cr;
    int dim = DIM, niter = ITERATION;
    // stats
    struct timeval t1, t2;
    int r, retries = 1, threads=4;
    double avg = 0, var, *runs;


    if (argc < 3) {
        printf("Usage: seq size niterations\n\n\n");
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

    tbb::task_scheduler_init init(threads);
    tbb::pipeline pipe;
    GenerateWork gw(dim);
    CalculateLine cl(dim, init_a, init_b, step, niter);
    RenderLine rl(dim);
    pipe.add_filter(gw);
    pipe.add_filter(cl);
    pipe.add_filter(rl);
    
#if !defined(NO_DISPLAY)
    SetupXWindows(dim, dim, 1, NULL, "Pipeline(Seq,seq,seq) Mandelbroot");
#endif

    for (r = 0; r < retries; r++) {

        // Start time
        gettimeofday(&t1, NULL);

        pipe.run(32);
        gw.reset();

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
    getchar();
    CloseXWindows();
#endif

    return 0;
}
