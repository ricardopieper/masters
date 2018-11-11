cmake_minimum_required(VERSION 3.12)
project(masters C)

set(CMAKE_C_STANDARD 11)

add_executable(masters
        mpi/pipeline_1.c
        mpi/pipeline_2.c
        mpi/pipeline_3.c
        mpi_masterslave_sort/main.c
        mpi_masterslave_raytracing/main.cpp)
