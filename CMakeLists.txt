
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
target_include_directories(deadlock_test PRIVATE src/algo)

add_executable(safestate_api
    src/api_server.cpp
    src/postgres_store.cpp
    src/scheduler.cpp
    src/algo/deadlock_detector.cpp
    src/algo/deadlock_recovery.cpp
    src/algo/rag_cycle_detection.cpp
    src/algo/multi_instance_detection.cpp
)

set(POSTGRESQL_ROOT "C:/Program Files/PostgreSQL/18" CACHE PATH
    "PostgreSQL installation directory")
target_include_directories(safestate_api PRIVATE "${POSTGRESQL_ROOT}/include")
target_link_directories(safestate_api PRIVATE "${POSTGRESQL_ROOT}/lib")
target_link_libraries(safestate_api PRIVATE libpq)
add_custom_command(TARGET safestate_api POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${POSTGRESQL_ROOT}/bin/libpq.dll"
        "$<TARGET_FILE_DIR:safestate_api>/libpq.dll")

if (WIN32)
    target_link_libraries(safestate_api PRIVATE ws2_32)
endif()