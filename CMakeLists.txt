
cmake_minimum_required(VERSION 3.16)

project(process_scheduling LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(process_scheduling
    src/main.cpp
    src/scheduler.cpp
    src/memory_management/memory_management.cpp
    src/memory_statistics/memory_statistics.cpp
)

add_executable(deadlock_test
    tests/deadlock_test_main.cpp
    src/algo/deadlock_detector.cpp
    src/algo/deadlock_recovery.cpp
    src/algo/rag_cycle_detection.cpp
    src/algo/multi_instance_detection.cpp
)

add_executable(safestate_api
    src/api_server.cpp
    src/scheduler.cpp
    src/algo/deadlock_detector.cpp
    src/algo/deadlock_recovery.cpp
    src/algo/rag_cycle_detection.cpp
    src/algo/multi_instance_detection.cpp
)

if (WIN32)
    target_link_libraries(safestate_api PRIVATE ws2_32)
endif()