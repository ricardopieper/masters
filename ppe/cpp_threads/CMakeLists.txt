cmake_minimum_required(VERSION 3.12)
project(cpp_threads)
find_package(Threads)
set(CMAKE_CXX_STANDARD 14)

add_executable(cpp_threads main.cpp parallel/StaticParallelFor.cpp parallel/StaticParallelFor.h parallel/DynamicParallelFor.cpp parallel/dalvan_queue.cpp parallel/Pipeline.cpp)

target_link_libraries (cpp_threads ${CMAKE_THREAD_LIBS_INIT})