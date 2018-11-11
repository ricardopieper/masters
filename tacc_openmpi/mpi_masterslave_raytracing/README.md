Compile:

    mpic++ -O3 main.cpp -o raytracer

Run:

    mpirun -np {WORKERS} raytracer

    ladrun -O3

Display result:
 1 - Install imagemagik
 2 - `display image.ppm`