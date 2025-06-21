#!/bin/cmake .

cmake_minimum_required(VERSION 3.30)

set(PROJECT_NAME RocketRuntime)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)

set(BINARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY})

#DebugMode
project(${PROJECT_NAME} CXX)

set(CMAKE_BUILD_TYPE Debug)

add_library(${PROJECT_NAME} STATIC
    src/window.cpp
    src/util.cpp
    src/renderer.cpp
    src/renderer3d.cpp
    src/io.cpp
    src/asset.cpp
    src/types.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE GL glfw GLU GLEW openal)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE RocketRuntime_DEBUG=1)
endif()

# Debug
project(${PROJECT_NAME}_test CXX)
add_executable(${PROJECT_NAME} tests/${PROJECT_NAME}.cpp)

target_link_directories(${PROJECT_NAME} PRIVATE bin)
target_include_directories(${PROJECT_NAME} PRIVATE include)

target_link_libraries(${PROJECT_NAME} PRIVATE RocketRuntime)

add_compile_options(-Wall -Wextra -Wpedantic -O3 -flto)
