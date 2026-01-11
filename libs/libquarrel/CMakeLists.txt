#!/bin/cmake .

cmake_minimum_required(VERSION 3.30)

set(PROJECT_NAME quarrel)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)

set(BINARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY})

#DebugMode
project(${PROJECT_NAME} CXX)

set(CMAKE_BUILD_TYPE Debug)

option(BUILD_SHARED_LIBS "Build Shared" ON)
option(BUILD_TESTS "Build Tests" ON)

add_library(${PROJECT_NAME}
    src/libquarrel.cpp
)

include_directories(include)

target_link_libraries(${PROJECT_NAME} PRIVATE)

if (BUILD_TESTS)
    set(TEST_NAMES
        args
    )

    foreach(test_name IN LISTS TEST_NAMES)
        add_executable(${test_name} tests/${test_name}.cpp)

        set_target_properties(${test_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/tests
        )

        target_link_directories(${test_name} PRIVATE bin)
        target_include_directories(${test_name} PRIVATE include)

        target_link_libraries(${test_name} PRIVATE quarrel)

        target_compile_options(${test_name} PRIVATE -Wall -Wextra -Wpedantic)
        set_target_properties(${test_name} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    endforeach()
endif()

message("Build Type: ${CMAKE_BUILD_TYPE}")
message("Build Shared: ${BUILD_SHARED_LIBS}")
message("Build Tests: ${BUILD_TESTS}")
if (BUILD_TESTS)
    message("Building Tests:")
    foreach(test_name IN LISTS TEST_NAMES)
        message("  ${test_name}")
    endforeach()
endif()
