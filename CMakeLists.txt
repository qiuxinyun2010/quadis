cmake_minimum_required (VERSION 3.0)
project(psmallocDemo)
include_directories(include)
add_executable(main src/psmalloc.cpp src/main.cpp)