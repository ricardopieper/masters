#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "mpi.h"
#include "../../../../../usr/local/include/mpi.h"
#include <math.h>
//The MPI usage of this program assumes MPI_Send and MPI_Recv are both blocking/synchronous.


#define GET_TASK_MSGTAG 2
#define NO_MORE_TASKS_MSGTAG 3
#define RESULT_MSGTAG 4
#define TASK_MSGTAG 4
#define MASTER 0
#define GRAINSIZE 16
#define ROWS 768
#define COLS 1024

#define SHOW_RESULTS


struct Vec {        // Usage: time ./explicit 16 && xv image.ppm
    double x, y, z;                  // position, also color (r,g,b)
    Vec(double x_ = 0, double y_ = 0, double z_ = 0) {
        x = x_;
        y = y_;
        z = z_;
    }

    Vec operator+(const Vec &b) const { return Vec(x + b.x, y + b.y, z + b.z); }

    Vec operator-(const Vec &b) const { return Vec(x - b.x, y - b.y, z - b.z); }

    Vec operator*(double b) const { return Vec(x * b, y * b, z * b); }

    Vec mult(const Vec &b) const { return Vec(x * b.x, y * b.y, z * b.z); }

    Vec &norm() { return *this = *this * (1 / sqrt(x * x + y * y + z * z)); }

    double dot(const Vec &b) const { return x * b.x + y * b.y + z * b.z; } // cross:
    Vec operator%(Vec &b) { return Vec(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x); }
};

struct Ray {
    Vec o, d;

    Ray(Vec o_, Vec d_) : o(o_), d(d_) {}
};

enum Refl_t {
    DIFF, SPEC, REFR
};  // material types, used in radiance()
struct Sphere {
    double rad;       // radius
    Vec p, e, c;      // position, emission, color
    Refl_t refl;      // reflection type (DIFFuse, SPECular, REFRactive)
    Sphere(double rad_, Vec p_, Vec e_, Vec c_, Refl_t refl_) :
            rad(rad_), p(p_), e(e_), c(c_), refl(refl_) {}

    double intersect(const Ray &r) const { // returns distance, 0 if nohit
        Vec op = p - r.o; // Solve t^2*d.d + 2*t*(o-p).d + (o-p).(o-p)-R^2 = 0
        double t, eps = 1e-4, b = op.dot(r.d), det = b * b - op.dot(op) + rad * rad;
        if (det < 0) return 0; else det = sqrt(det);
        return (t = b - det) > eps ? t : ((t = b + det) > eps ? t : 0);
    }
};

Sphere spheres[] = {//Scene: radius, position, emission, color, material
        Sphere(1e5, Vec(1e5 + 1, 40.8, 81.6), Vec(), Vec(.75, .25, .25), DIFF),//Left
        Sphere(1e5, Vec(-1e5 + 99, 40.8, 81.6), Vec(), Vec(.25, .25, .75), DIFF),//Rght
        Sphere(1e5, Vec(50, 40.8, 1e5), Vec(), Vec(.75, .75, .75), DIFF),//Back
        Sphere(1e5, Vec(50, 40.8, -1e5 + 170), Vec(), Vec(), DIFF),//Frnt
        Sphere(1e5, Vec(50, 1e5, 81.6), Vec(), Vec(.75, .75, .75), DIFF),//Botm
        Sphere(1e5, Vec(50, -1e5 + 81.6, 81.6), Vec(), Vec(.75, .75, .75), DIFF),//Top
        Sphere(16.5, Vec(27, 16.5, 47), Vec(), Vec(1, 1, 1) * .999, SPEC),//Mirr
        Sphere(16.5, Vec(73, 16.5, 78), Vec(), Vec(1, 1, 1) * .999, REFR),//Glas
        Sphere(1.5, Vec(50, 81.6 - 16.5, 81.6), Vec(4, 4, 4) * 100, Vec(), DIFF),//Lite
};
int numSpheres = sizeof(spheres) / sizeof(Sphere);

inline double clamp(double x) { return x < 0 ? 0 : x > 1 ? 1 : x; }

inline int toInt(double x) { return int(pow(clamp(x), 1 / 2.2) * 255 + .5); }

inline bool intersect(const Ray &r, double &t, int &id) {
    double n = sizeof(spheres) / sizeof(Sphere), d, inf = t = 1e20;
    for (int i = int(n); i--;)
        if ((d = spheres[i].intersect(r)) && d < t) {
            t = d;
            id = i;
        }
    return t < inf;
}

Vec radiance(const Ray &r, int depth, unsigned short *Xi, int E = 1) {
    double t;                               // distance to intersection
    int id = 0;                               // id of intersected object
    if (!intersect(r, t, id)) return Vec(); // if miss, return black
    const Sphere &obj = spheres[id];        // the hit object
    Vec x = r.o + r.d * t, n = (x - obj.p).norm(), nl = n.dot(r.d) < 0 ? n : n * -1, f = obj.c;
    double p = f.x > f.y && f.x > f.z ? f.x : f.y > f.z ? f.y : f.z; // max refl
    if (++depth > 5 || !p) if (erand48(Xi) < p) f = f * (1 / p); else return obj.e * E;
    if (obj.refl == DIFF) {                  // Ideal DIFFUSE reflection
        double r1 = 2 * M_PI * erand48(Xi), r2 = erand48(Xi), r2s = sqrt(r2);
        Vec w = nl, u = ((fabs(w.x) > .1 ? Vec(0, 1) : Vec(1)) % w).norm(), v = w % u;
        Vec d = (u * cos(r1) * r2s + v * sin(r1) * r2s + w * sqrt(1 - r2)).norm();

        // Loop over any lights
        Vec e;
        for (int i = 0; i < numSpheres; i++) {
            const Sphere &s = spheres[i];
            if (s.e.x <= 0 && s.e.y <= 0 && s.e.z <= 0) continue; // skip non-lights

            Vec sw = s.p - x, su = ((fabs(sw.x) > .1 ? Vec(0, 1) : Vec(1)) % sw).norm(), sv = sw % su;
            double cos_a_max = sqrt(1 - s.rad * s.rad / (x - s.p).dot(x - s.p));
            double eps1 = erand48(Xi), eps2 = erand48(Xi);
            double cos_a = 1 - eps1 + eps1 * cos_a_max;
            double sin_a = sqrt(1 - cos_a * cos_a);
            double phi = 2 * M_PI * eps2;
            Vec l = su * cos(phi) * sin_a + sv * sin(phi) * sin_a + sw * cos_a;
            l.norm();
            if (intersect(Ray(x, l), t, id) && id == i) {  // shadow ray
                double omega = 2 * M_PI * (1 - cos_a_max);
                e = e + f.mult(s.e * l.dot(nl) * omega) * M_1_PI;  // 1/pi for brdf
            }
        }

        return obj.e * E + e + f.mult(radiance(Ray(x, d), depth, Xi, 0));
    } else if (obj.refl == SPEC)              // Ideal SPECULAR reflection
        return obj.e + f.mult(radiance(Ray(x, r.d - n * 2 * n.dot(r.d)), depth, Xi));
    Ray reflRay(x, r.d - n * 2 * n.dot(r.d));     // Ideal dielectric REFRACTION
    bool into = n.dot(nl) > 0;                // Ray from outside going in?
    double nc = 1, nt = 1.5, nnt = into ? nc / nt : nt / nc, ddn = r.d.dot(nl), cos2t;
    if ((cos2t = 1 - nnt * nnt * (1 - ddn * ddn)) < 0)    // Total internal reflection
        return obj.e + f.mult(radiance(reflRay, depth, Xi));
    Vec tdir = (r.d * nnt - n * ((into ? 1 : -1) * (ddn * nnt + sqrt(cos2t)))).norm();
    double a = nt - nc, b = nt + nc, R0 = a * a / (b * b), c = 1 - (into ? -ddn : tdir.dot(n));
    double Re = R0 + (1 - R0) * c * c * c * c * c, Tr = 1 - Re, P = .25 + .5 * Re, RP = Re / P, TP = Tr / (1 - P);
    return obj.e + f.mult(depth > 2 ? (erand48(Xi) < P ?   // Russian roulette
                                       radiance(reflRay, depth, Xi) * RP : radiance(Ray(x, tdir), depth, Xi) * TP) :
                          radiance(reflRay, depth, Xi) * Re + radiance(Ray(x, tdir), depth, Xi) * Tr);
}

typedef struct Task {
    int lineStart;
    int lineEnd;
} Task;


int *serialize_task(const Task &task, int *totalCount) {
    int *serialized = new int[1];
    serialized[0] = task.lineStart;
    serialized[1] = task.lineEnd;
    *totalCount = 2;
    return serialized;
};

Task deserialize_task(int *taskData) {
    Task r;
    r.lineStart = taskData[0];
    r.lineEnd = taskData[1];
    return r;
};


double *serialize_result(int line, Vec *vec, int vecSize, int *size) {

    *size = (vecSize * 3) + 1;

    double *serialized = new double[*size];

    serialized[0] = line;

    for (int i = 0; i < vecSize; i++) {
        int j = i * 3 + 1;
        serialized[j] = vec[i].x;
        serialized[j + 1] = vec[i].y;
        serialized[j + 2] = vec[i].z;
        //     printf("rgb  is %f %f %f at line %i row %i\n", vec[i].x, vec[i].y, vec[i].z, line, i);
    }
    delete[] vec;

    return serialized;
}


void renderImage(double **results) {
#ifdef SHOW_RESULTS

    FILE *f = fopen("image.ppm", "w"); // Write image to PPM file.
    fprintf(f, "P3\n%d %d\n%d\n", COLS, ROWS, 255);
    for (int i = ROWS - 1; i >= 0; i--) {
        double *row = results[i];
        for (int j = 0; j < COLS * 3; j += 3) {
            fprintf(f, "%d %d %d ", toInt(row[j]), toInt(row[j + 1]), toInt(row[j + 2]));
        }
    }
#endif
}


double *raytrace_line(int y, int w, int h, int samps, int *bufferSize) {
    Ray cam(Vec(50, 52, 295.6), Vec(0, -0.042612, -1).norm()); // cam pos, dir
    Vec cx = Vec(w * .5135 / h), cy = (cx % cam.d).norm() * .5135, r;

    Vec *c = new Vec[w];
  //  fprintf(stderr, "\rRendering (%d spp) %5.2f%%", samps * 4, 100. * y / (h - 1));
    for (unsigned short x = 0, Xi[3] = {0, 0, y * y * y}; x < w; x++)   // Loop cols
        for (int sy = 0; sy < 2; sy++) {   // 2x2 subpixel rows
            for (int sx = 0; sx < 2; sx++, r = Vec()) {        // 2x2 subpixel cols
                for (int s = 0; s < samps; s++) {

                    double r1 = 2 * erand48(Xi), dx = r1 < 1 ? sqrt(r1) - 1 : 1 - sqrt(2 - r1);
                    double r2 = 2 * erand48(Xi), dy = r2 < 1 ? sqrt(r2) - 1 : 1 - sqrt(2 - r2);
                    Vec d = cx * (((sx + .5 + dx) / 2 + x) / w - .5) +
                            cy * (((sy + .5 + dy) / 2 + y) / h - .5) + cam.d;
                    r = r + radiance(Ray(cam.o + d * 140, d.norm()), 0, Xi) * (1. / samps);


                } // Camera rays are pushed ^^^^^ forward to start in interior
                c[x] = c[x] + Vec(clamp(r.x), clamp(r.y), clamp(r.z)) * .25;
            }
        }

    return serialize_result(y, c, w, bufferSize);
}


void raytrace_lines2(Task t, double *buffer, int w, int h, int samps) {
    Ray cam(Vec(50, 52, 295.6), Vec(0, -0.042612, -1).norm()); // cam pos, dir
    Vec cx = Vec(w * .5135 / h), cy = (cx % cam.d).norm() * .5135, r;

    for (int y = t.lineStart, line = 0; y <= t.lineEnd; y++, line++) {
       // fprintf(stderr, "\rRendering (%d spp) %5.2f%%", samps * 4, 100. * y / (h - 1));
        for (unsigned short x = 0, Xi[3] = {0, 0, y * y * y}; x < w; x++) {  // Loop cols
            Vec c;
            for (int sy = 0; sy < 2; sy++) {   // 2x2 subpixel rows
                for (int sx = 0; sx < 2; sx++, r = Vec()) {        // 2x2 subpixel cols
                    for (int s = 0; s < samps; s++) {

                        double r1 = 2 * erand48(Xi), dx = r1 < 1 ? sqrt(r1) - 1 : 1 - sqrt(2 - r1);
                        double r2 = 2 * erand48(Xi), dy = r2 < 1 ? sqrt(r2) - 1 : 1 - sqrt(2 - r2);
                        Vec d = cx * (((sx + .5 + dx) / 2 + x) / w - .5) +
                                cy * (((sy + .5 + dy) / 2 + y) / h - .5) + cam.d;
                        r = r + radiance(Ray(cam.o + d * 140, d.norm()), 0, Xi) * (1. / samps);


                    } // Camera rays are pushed ^^^^^ forward to start in interior
                    c = c + Vec(clamp(r.x), clamp(r.y), clamp(r.z)) * .25;
                }
            }
            int pixel = 3 * ((line * w) + x);
            buffer[pixel] = c.x;
            buffer[pixel + 1] = c.y;
            buffer[pixel + 2] = c.z;
        }
    }

}


void runMasterLoop(int numberOfProcesses) {
    int dummy = 0;
    MPI_Status status;

    double **allRenderedLines = new double *[ROWS];

    Task *runningTasks = new Task[numberOfProcesses];


    int nextTask = 0;
    int completedTasks = 0;

    while (completedTasks < (ROWS / GRAINSIZE)) {
        //Receive a message of any type

        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        //after probing what kind of message we're getting, take appropriate action

        //if a slave is asking for work
        if (status.MPI_TAG == GET_TASK_MSGTAG) {
            //finish receiving message (throws message away)
            MPI_Recv(&dummy, 1, MPI_INT, status.MPI_SOURCE, GET_TASK_MSGTAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (nextTask < ROWS) {
                //prepare his tasks
                Task t;
                t.lineStart = nextTask; //0
                t.lineEnd = nextTask + GRAINSIZE - 1; //1

                runningTasks[status.MPI_SOURCE - 1] = t;

                int taskSize;
                int *buf = serialize_task(t, &taskSize);

                MPI_Send(buf, taskSize, MPI_INT, status.MPI_SOURCE, TASK_MSGTAG, MPI_COMM_WORLD);
                //now that the send is done, free the buffer
                free(buf);
                nextTask += GRAINSIZE;
            } //we have no more tasks for them... keep them waiting for a bit
        } else if (status.MPI_TAG == RESULT_MSGTAG) {
            //we received a result back from a slave. Print the result
            //and free the task buffer we allocated previously
            //first, get the result buffer size
            int bufLength;
            MPI_Get_count(&status, MPI_DOUBLE, &bufLength);
            //allocate only what's necessary
            double *recvBuffer = new double[bufLength];

            MPI_Recv(recvBuffer, bufLength, MPI_DOUBLE, status.MPI_SOURCE, RESULT_MSGTAG, MPI_COMM_WORLD, &status);

            Task t = runningTasks[status.MPI_SOURCE - 1];

            for (int i = t.lineStart, bufferLine = 0; i <= t.lineEnd; i++, bufferLine++) {
                allRenderedLines[i] = &recvBuffer[bufferLine * COLS * 3];
            }

            //do bookkeeping
            completedTasks++;
        }

    }

    //now that all tasks are complete, release slaves from their misery
    for (int i = 1; i < numberOfProcesses; i++) {
        printf("Master: Releasing slave %i\n", i);

        MPI_Send(&dummy, 1, MPI_INT, i, NO_MORE_TASKS_MSGTAG, MPI_COMM_WORLD);
    }

    //and print everything

    renderImage(allRenderedLines);

}

void runSlaveLoop() {
    int dummy = 0;
    MPI_Status status;
    //work forever until master releases us
    while (1) {
        //ask for work
        MPI_Send(&dummy, 1, MPI_INT, MASTER, GET_TASK_MSGTAG, MPI_COMM_WORLD);
        //master can either give a task or release us
        //figure out what master wants
        MPI_Probe(MASTER, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if (status.MPI_TAG == NO_MORE_TASKS_MSGTAG) {
            break;
        } else if (status.MPI_TAG == TASK_MSGTAG) {
            //we have received a task
            int count;
            MPI_Get_count(&status, MPI_INT, &count);
            int *taskBuffer = new int[count];
            MPI_Recv(taskBuffer, count, MPI_INT, MASTER, TASK_MSGTAG, MPI_COMM_WORLD, &status);
            Task t = deserialize_task(taskBuffer);

            //ray-trace
            int lines = (t.lineEnd - t.lineStart) + 1;

            int bufferSize = COLS * lines * 3;

            double *resultBuffer = new double[bufferSize];

            raytrace_lines2(t, resultBuffer, COLS, ROWS, 1);

            //communicate the result back to master
            MPI_Send(resultBuffer, bufferSize, MPI_DOUBLE, 0, RESULT_MSGTAG, MPI_COMM_WORLD);

            delete[] resultBuffer;
            delete[] taskBuffer;
        }
    }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int myRank;
    int totalProcesses;

    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    MPI_Comm_size(MPI_COMM_WORLD, &totalProcesses);

    if (myRank == 0) {
        printf("Starting as master.... \n");
        //master will only do task scheduling
        runMasterLoop(totalProcesses);
    } else {
        printf("Starting as slave.... \n");
        runSlaveLoop();
    }

    MPI_Finalize();
    return 0;
}