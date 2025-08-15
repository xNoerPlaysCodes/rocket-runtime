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

option(BUILD_SHARED_LIBS "Build Shared" ON)
option(BUILD_TESTS "Build Tests" ON)

option(BUILD_QUARK "Build Quark [physics]" ON)
option(BUILD_ASTRO "Build Astro [ui]" ON)

if (BUILD_QUARK)
    set(QUARK_SOURCES 
    )
    add_compile_definitions(ROCKETGE__BUILD_QUARK)
endif()

if (BUILD_ASTRO)
    set(ASTRO_SOURCES
        src/astro/astroui.cpp
    )
    add_compile_definitions(ROCKETGE__BUILD_ASTRO)
endif()

add_library(${PROJECT_NAME}
    src/rocket/window.cpp
    src/rocket/util.cpp
    src/rocket/renderer.cpp
    src/rocket/renderer3d.cpp
    src/rocket/io.cpp
    src/rocket/asset.cpp
    src/rocket/types.cpp
    src/rocket/runtime.cpp

    src/rocket/font.cpp
    src/rocket/text.cpp

    src/rocket/shader.cpp

    # AstroUI
    ${ASTRO_SOURCES}

    # Quark
    ${QUARK_SOURCES}
)

include_directories(
    /usr/include/freetype2
    /usr/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE GL glfw GLU GLEW openal freetype)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE RocketRuntime_DEBUG=1)
endif()

if (BUILD_TESTS)
    set(TEST_NAMES
        rectangle_test
        initializer_test
        text_test
        ui_init_test
        shader_test
        rounded_texture_rectangle_test
    )

    foreach(test_name IN LISTS TEST_NAMES)
        add_executable(${test_name} tests/${test_name}.cpp)

        target_link_directories(${test_name} PRIVATE bin)
        target_include_directories(${test_name} PRIVATE include)

        target_link_libraries(${test_name} PRIVATE RocketRuntime)

        target_compile_options(${test_name} PRIVATE -Wall -Wextra -Wpedantic -O3 -flto)
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
message("Build Quark: ${BUILD_QUARK}")
message("Build Astro: ${BUILD_ASTRO}")
