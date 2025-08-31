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
option(BUILD_EXAMPLES "Build Examples" ON)

option(BUILD_QUARK "Build Quark [physics]" ON)
option(BUILD_ASTRO "Build Astro [ui]" ON)

include_directories(include)
include_directories(src/include)

if (BUILD_QUARK)
    set(QUARK_SOURCES 
        src/quark/physics.cpp
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

    src/rocket/rgl.cpp

    # AstroUI
    ${ASTRO_SOURCES}

    # Quark
    ${QUARK_SOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE GL glfw GLU GLEW openal)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE RocketRuntime_DEBUG=1)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -O3 -flto)
endif()

if (BUILD_TESTS)
    set(TEST_NAMES
        rectangle_test
        initializer_test
        text_test
        ui_init_test
        rounded_texture_rectangle_test
        line_test
    )

    foreach(test_name IN LISTS TEST_NAMES)
        add_executable(${test_name} tests/${test_name}.cpp)

        set_target_properties(${test_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/tests
        )

        target_link_directories(${test_name} PRIVATE bin)
        target_include_directories(${test_name} PRIVATE include)

        target_link_libraries(${test_name} PRIVATE RocketRuntime)

        target_compile_options(${test_name} PRIVATE -Wall -Wextra -Wpedantic -O3 -flto)
    endforeach()
endif()

if (BUILD_EXAMPLES)
    set(EXAMPLE_NAMES
        moving_circle
        custom_shader
    )

    foreach(example_name IN LISTS EXAMPLE_NAMES)
        add_executable(${example_name} examples/${example_name}.cpp)

        set_target_properties(${example_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/examples
        )

        target_link_directories(${example_name} PRIVATE bin)
        target_include_directories(${example_name} PRIVATE include)

        target_link_libraries(${example_name} PRIVATE RocketRuntime)

        target_compile_options(${example_name} PRIVATE -Wall -Wextra -Wpedantic -O3 -flto)
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
message("Build Examples: ${BUILD_EXAMPLES}")
if (BUILD_EXAMPLES)
    message("Building Examples:")
    foreach(example_name IN LISTS EXAMPLE_NAMES)
        message("  ${example_name}")
    endforeach()
endif()
message("Build Quark: ${BUILD_QUARK}")
message("Build Astro: ${BUILD_ASTRO}")
