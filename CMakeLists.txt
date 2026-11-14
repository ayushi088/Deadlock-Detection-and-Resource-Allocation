
cmake_minimum_required(VERSION 3.16)
project(process_scheduling LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(process_scheduling
    src/main.cpp
    src/scheduler.cpp
    src/memory_management.cpp
)
