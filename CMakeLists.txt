#!/bin/cmake .

cmake_minimum_required(VERSION 3.30)

set(PROJECT_NAME RocketRuntime)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)

set(BINARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY})

#DebugMode
project(${PROJECT_NAME} CXX)

set(CMAKE_BUILD_TYPE Release)

option(BUILD_SHARED_LIBS "Build Shared" ON)
option(BUILD_TESTS "Build Tests" ON)
option(BUILD_EXAMPLES "Build Examples" ON)

option(BUILD_QUARK "Build Quark [physics]" ON)
option(BUILD_ASTRO "Build Astro [ui]" ON)
option(GLFNLDR_BACKEND "GLfnldr backend [GLEW/LIBEPOXY]" "GLEW")

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

if (GLFNLDR_BACKEND STREQUAL "GLEW")
    add_compile_definitions(ROCKETGE__GLFNLDR_BACKEND_GLEW)
    add_compile_definitions(ROCKETGE__GLFNLDR_BACKEND_ENUM=::glfnldr::backend_t::glew)
elseif(GLFNLDR_BACKEND STREQUAL "LIBEPOXY")
    add_compile_definitions(ROCKETGE__GLFNLDR_BACKEND_LIBEPOXY)
    add_compile_definitions(ROCKETGE__GLFNLDR_BACKEND_ENUM=::glfnldr::backend_t::libepoxy)
else()
    message(FATAL_ERROR "Unknown GLfnldr backend: ${GLFNLDR_BACKEND}, you must compile with 1 valid backend")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(RNATIVE_SOURCES
        src/rnative/xdg-toplevel-icon-v1-client-protocol.c
    )
endif()

add_library(${PROJECT_NAME}
    # Graphics
    src/rocket/gfx/renderer.cpp
    # src/rocket/gfx/renderer3d.cpp
    src/rocket/gfx/rgl.cpp

    # ManagedAssets
    src/rocket/managers/assets/font.cpp
    src/rocket/managers/assets/shader.cpp
    src/rocket/managers/assets/text.cpp
    # Managers
    src/rocket/managers/asset.cpp
    src/rocket/managers/audio.cpp

    # Utilities
    src/rocket/util/glfnldr.cpp
    src/rocket/util/io.cpp
    src/rocket/util/native.cpp
    src/rocket/util/threads.cpp
    src/rocket/util/types.cpp
    src/rocket/util/crashdump.cpp
    src/rocket/util/shader_provider.cpp
    src/rocket/util/persistence.cpp
    src/rocket/util/scripting.cpp
    src/rocket/util/util.cpp

    # Master
    src/rocket/window.cpp
    src/rocket/runtime.cpp

    # Plugin Subsystem
    src/rocket/plugin/plugin.cpp

    # Header-Only Libraries Implementation
    src/headeronlyimpl.cpp

    # RNative
    ${RNATIVE_SOURCES}

    # AstroUI
    ${ASTRO_SOURCES}

    # Quark
    ${QUARK_SOURCES}
)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE include)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE /usr/include/miniz)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE src/include)

# Fetch Dependencies
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(PkgConfig REQUIRED)
if (GLFNLDR_BACKEND STREQUAL "GLEW")
    find_package(GLEW REQUIRED)
elseif(GLFNLDR_BACKEND STREQUAL "LIBEPOXY")
    pkg_check_modules(EPOXY REQUIRED epoxy)
endif()
find_package(OpenAL REQUIRED)
find_package(miniz REQUIRED) # miniz may not be installed
find_package(SDL2 REQUIRED)
find_package(pybind11 REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL ${EPOXY_LIBRARIES} GLEW::GLEW glfw OpenAL::OpenAL miniz SDL2::SDL2 Python3::Python)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE RocketRuntime_DEBUG=1) # Legacy definition
    target_compile_definitions(${PROJECT_NAME} PRIVATE ROCKETGE__DEBUG_BUILD)
    target_compile_options(${PROJECT_NAME} PRIVATE -O0 -g -Wall)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -O3 -flto)
    set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if (BUILD_TESTS)
    set(TEST_NAMES
        initializer_test
        text_test
        gamepad_test
        icon_test
        io_simulation_test
        io_test
        plugin_test
        multithreaded_test
        sound_engine_test
        borderless_windowed_test
        triangle_drawcall_test
        default_shader_test
        persistence_test
        scripting_test
    )

    if (BUILD_ASTRO)
        set(TEST_NAMES ${TEST_NAMES}
            ui_init_test
        )
    endif()

    foreach(test_name IN LISTS TEST_NAMES)
        option(BUILD_TEST_${test_name} "Build Test ${test_name}" ON)
        if (BUILD_TEST_${test_name})
            add_executable(${test_name} tests/${test_name}.cpp)

            set_target_properties(${test_name} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/tests
            )

            target_link_directories(${test_name} PRIVATE bin)
            target_include_directories(${test_name} PRIVATE include)

            target_link_libraries(${test_name} PRIVATE RocketRuntime Python3::Python)

            target_compile_options(${test_name} PRIVATE -Wall -Wextra -Wpedantic)
            set_target_properties(${test_name} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
        else()
            message("Test skipped: ${test_name}")
        endif()
    endforeach()
endif()

if (BUILD_EXAMPLES)
    set(EXAMPLE_NAMES
        moving_circle
        custom_shader
        software_renderer
    )

    foreach(example_name IN LISTS EXAMPLE_NAMES)
        option(BUILD_EXAMPLE_${example_name} "Build Example ${example_name}" ON)
        if (BUILD_EXAMPLE_${example_name})
            add_executable(${example_name} examples/${example_name}.cpp)

            set_target_properties(${example_name} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/examples
            )

            target_link_directories(${example_name} PRIVATE bin)
            target_include_directories(${example_name} PRIVATE include)

            target_link_libraries(${example_name} PRIVATE RocketRuntime)

            target_compile_options(${example_name} PRIVATE -Wall -Wextra -Wpedantic -O3)
            set_target_properties(${example_name} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
        else()
            message("Skip example: ${example_name}")
        endif()
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
