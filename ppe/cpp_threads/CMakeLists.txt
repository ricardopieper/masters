cmake_minimum_required(VERSION 3.10)
project(cpp_threads)
find_package(Threads)
find_package(X11 REQUIRED)
set(CMAKE_CXX_STANDARD 14)


message(STATUS "X11_FOUND = ${X11_FOUND}")
message(STATUS "X11_INCLUDE_DIR = ${X11_INCLUDE_DIR}")
message(STATUS "X11_LIBRARIES = ${X11_LIBRARIES}")

include_directories(${X11_INCLUDE_DIR})

link_directories(${X11_LIBRARIES})

add_executable(cpp_threads
        main.cpp
        parallel/dalvan_queue.cpp
        parallel/Pipeline.cpp)


add_executable(pipeline_farm
        mandel_pipeline_farm.cpp
        parallel/dalvan_queue.cpp
        marX2.h
        marX2.c
        parallel/Pipeline.cpp)


target_link_libraries (cpp_threads ${CMAKE_THREAD_LIBS_INIT})

target_link_libraries (pipeline_farm ${CMAKE_THREAD_LIBS_INIT} ${X11_LIBRARIES})