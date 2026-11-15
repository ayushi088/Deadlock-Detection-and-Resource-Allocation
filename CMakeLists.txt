cmake_minimum_required(VERSION 3.16)
project(process_scheduling LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(process_scheduling
    src/main.cpp
    src/scheduler.cpp
)

add_executable(deadlock_test
    tests/deadlock_test_main.cpp
    algo/deadlock_detector.cpp
    algo/deadlock_recovery.cpp
    algo/rag_cycle_detection.cpp
    algo/multi_instance_detection.cpp
)