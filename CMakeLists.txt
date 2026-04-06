#!/bin/cmake .

cmake_minimum_required(VERSION 3.30)

set(PROJECT_NAME RocketRuntime)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(BINARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY})

#DebugMode
project(${PROJECT_NAME} CXX C)

set(CMAKE_BUILD_TYPE Debug)

option(BUILD_SHARED_LIBS "Build Shared" ON)
option(BUILD_TESTS "Build Tests" ON)
option(BUILD_EXAMPLES "Build Examples" ON)
option(BUILD_EDITOR "Build Editor" OFF)
option(__rge_WINDOWS__ OFF)
option(__rge_ANDROID__ OFF)

if (WIN32)
    set(__rge_WINDOWS__ ON)
elseif (ANDROID)
    set(__rge_ANDROID__ ON)
endif()

option(BUILD_ASTRO "Build Astro [ui]" ON)
option(BUILD_SCRIPTING "Build Scripting [py]" ON)
# option(GLFNLDR_BACKEND "GLfnldr backend [GLEW/LIBEPOXY]" "GLEW")

if (BUILD_ASTRO)
    set(ASTRO_SOURCES
        src/astro/astroui.cpp
    )
    add_compile_definitions(ROCKETGE__BUILD_ASTRO)
endif()

if (BUILD_SCRIPTING)
    set(SCRIPTING_SOURCES
        src/rocket/util/scripting.cpp
    )
    add_compile_definitions(ROCKETGE__BUILD_SCRIPTING)
endif()

if (GLFNLDR_BACKEND STREQUAL "GLEW")
    add_compile_definitions(ROCKETGE__GLFNLDR_BACKEND_GLEW)
    add_compile_definitions(ROCKETGE__GLFNLDR_BACKEND_ENUM=::glfnldr::backend_t::glew)
elseif(GLFNLDR_BACKEND STREQUAL "LIBEPOXY")
    add_compile_definitions(ROCKETGE__GLFNLDR_BACKEND_LIBEPOXY)
    add_compile_definitions(ROCKETGE__GLFNLDR_BACKEND_ENUM=::glfnldr::backend_t::libepoxy)
elseif(GLFNLDR_BACKEND STREQUAL "ANDROID")
    add_compile_definitions(ROCKETGE__GLFNLDR_BACKEND_ANDROID)
    add_compile_definitions(ROCKETGE__GLFNLDR_BACKEND_ENUM=::glfnldr::backend_t::android)
else()
    message(WARNING "Unknown GLfnldr backend: ${GLFNLDR_BACKEND}, compiling with GLEW")
    add_compile_definitions(ROCKETGE__GLFNLDR_BACKEND_GLEW)
    add_compile_definitions(ROCKETGE__GLFNLDR_BACKEND_ENUM=::glfnldr::backend_t::glew)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(RNATIVE_SOURCES
        src/rnative/xdg-toplevel-icon-v1-client-protocol.c
        src/rnative/xdg-shell-client-protocol.c
    )
endif()

set(WINDOWING_SOURCES
    src/rocket/core/window/glfw_window.cpp
    src/rocket/core/window/android_app.cpp
)

set(GLFNLDR_SOURCES
    src/rocket/util/glfnldr/glad.cpp
    src/rocket/util/glfnldr/glew.cpp
    src/rocket/util/glfnldr/libepoxy.cpp
)

if (__rge_ANDROID__)
    set(WINDOWING_SOURCES
       src/rocket/core/window/android_app.cpp
    )

    set(GLFNLDR_SOURCES
        src/rocket/util/glfnldr/android.cpp
    )
endif()

set(ROCKETGE_SOURCES
    # Graphics
    src/rocket/gfx/renderer/opengl_renderer.cpp
    src/rocket/gfx/renderer/null.cpp
    src/rocket/gfx/renderer/vulkan_renderer.cpp
    # src/rocket/gfx/renderer3d.cpp
    src/rocket/gfx/helper/rgl.cpp

    # ManagedAssets
    src/rocket/managers/assets/font.cpp
    src/rocket/managers/assets/shader/opengl_shader.cpp
    src/rocket/managers/assets/shader/vulkan_shader.cpp
    src/rocket/managers/assets/text.cpp
    # Managers
    src/rocket/managers/asset.cpp
    src/rocket/managers/audio.cpp

    # Utilities
    src/rocket/util/glfnldr/glfnldr.cpp
    ${GLFNLDR_SOURCES}
    src/rocket/util/io.cpp
    src/rocket/util/native.cpp
    src/rocket/util/threads.cpp
    src/rocket/util/types.cpp
    src/rocket/util/crashdump.cpp
    src/rocket/util/shader_provider.cpp
    src/rocket/util/persistence.cpp
    src/rocket/util/data_structures.cpp
    src/rocket/util/memory.cpp
    src/rocket/util/rlsl_parser.cpp
    src/rocket/util/util.cpp

    # Internal
    src/rocket/internal/internal_types.cpp

    # Core
    ${WINDOWING_SOURCES}
    src/rocket/core/window/null.cpp
    src/rocket/core/window.cpp
    src/rocket/core/runtime.cpp

    # Plugin Subsystem
    src/rocket/plugin/plugin.cpp

    # Special Cases
    src/libs.cpp

    # RNative
    ${RNATIVE_SOURCES}

    # AstroUI
    ${ASTRO_SOURCES}

    # Python Scripting
    ${SCRIPTING_SOURCES}
)

if (WIN32)
    # Force static on Windows to avoid declspec headaches
    add_library(${PROJECT_NAME} STATIC
        ${ROCKETGE_SOURCES}
    )

else()
    add_library(${PROJECT_NAME}
        ${ROCKETGE_SOURCES}
    )
endif()

if (__rge_ANDROID__)
    add_library(native_app_glue STATIC
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
    )
    set_target_properties(native_app_glue PROPERTIES
        LANGUAGE C
    )
    target_include_directories(native_app_glue PUBLIC
        ${ANDROID_NDK}/sources/android/native_app_glue
    )
endif()

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE include)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE /usr/include/miniz)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE src/include)

file(GLOB SHADER_HEADERS
    "${CMAKE_SOURCE_DIR}/src/include/resources/shader_*.h"
)

set(SHADER_NAME_LIST "")
set(SHADER_ID_LIST "")
set(SHADER_INCLUDE_LINES "")
set(SHADER_DISPATCH_ENTRIES "")

foreach(SHADER_PATH ${SHADER_HEADERS})
    get_filename_component(SHADER_NAME     ${SHADER_PATH} NAME_WE)
    get_filename_component(SHADER_FILENAME ${SHADER_PATH} NAME)

    string(REGEX REPLACE "^shader_" "" SHORT_NAME ${SHADER_NAME})

    list(APPEND SHADER_NAME_LIST "${SHADER_NAME}_rlsl")
    list(APPEND SHADER_ID_LIST   "shader_id_t::${SHORT_NAME}")

    string(APPEND SHADER_INCLUDE_LINES   "#include \"${SHADER_FILENAME}\"\n")
    string(APPEND SHADER_DISPATCH_ENTRIES "SHADER_DISPATCH_ENTRY(${SHORT_NAME})\n")
endforeach()

list(JOIN SHADER_NAME_LIST ", " SHADER_NAMES_JOINED)
list(JOIN SHADER_ID_LIST   ", " SHADER_IDS_JOINED)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    "RGE_SHADER_REGISTRY_SHADERS={ ${SHADER_NAMES_JOINED} }"
    "SHADER_ID_LIST=${SHADER_IDS_JOINED}"
)

file(GENERATE
    OUTPUT "${CMAKE_SOURCE_DIR}/src/include/resources/autogen_shader_includes.h"
    CONTENT "// Autogenerated by CMake\n// DO NOT MODIFY!\n${SHADER_INCLUDE_LINES}"
)

file(GENERATE
    OUTPUT "${CMAKE_SOURCE_DIR}/src/include/resources/autogen_shader_dispatch.h"
    CONTENT "${SHADER_DISPATCH_ENTRIES}"
)

if (BUILD_SCRIPTING)
    set(PYTHON_LIB Python3::Python)
    set(PYBIND11_LIB pybind11::pybind11)
endif()

# Fetch Dependencies
if (__rge_WINDOWS__)
    message("Building for Win32")
    message("RocketGE Platform: Desktop-GLFW-Win32")

    find_package(OpenGL REQUIRED)
    find_package(GLEW CONFIG REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(OpenAL CONFIG REQUIRED)
    if (BUILD_SCRIPTING)
        find_package(Python3 COMPONENTS Development REQUIRED)
        find_package(pybind11 CONFIG REQUIRED)
    endif()
    find_package(glm CONFIG REQUIRED)
    find_package(nlohmann_json CONFIG REQUIRED)
    find_package(miniz CONFIG REQUIRED)
    find_package(Vulkan REQUIRED)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        OpenGL::GL
        GLEW::GLEW
        glfw
        SDL2::SDL2
        OpenAL::OpenAL
        ${PYTHON_LIB}
        nlohmann_json::nlohmann_json
        miniz::miniz
        Vulkan::Vulkan
        winmm
    )
    target_link_libraries(${PROJECT_NAME}
        PUBLIC
        ${PYBIND11_LIB}
    )
elseif (__rge_ANDROID__)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/androiddeps/libs/${ANDROID_ABI}/libopenal.a
        ${CMAKE_SOURCE_DIR}/androiddeps/libs/${ANDROID_ABI}/libSDL2.a
        ${CMAKE_SOURCE_DIR}/androiddeps/libs/${ANDROID_ABI}/libminiz.a
        android
        log
        EGL
        GLESv3
        OpenSLES
        vulkan
    )
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/androiddeps/include
        ${ANDROID_NDK}/sources/android/native_app_glue
    )
    message("RocketGE Platform: Mobile-Embedded-Android")
else()
    message("Building for Other")

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
    find_package(Vulkan REQUIRED)
    if (BUILD_SCRIPTING)
        find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
    endif()
    pkg_check_modules(WAYLAND REQUIRED wayland-client)

    target_link_libraries(${PROJECT_NAME} PRIVATE 
        OpenGL::GL 
        ${EPOXY_LIBRARIES} 
        GLEW::GLEW 
        glfw 
        OpenAL::OpenAL 
        miniz 
        SDL2::SDL2 
        ${PYTHON_LIB} 
        stdc++exp 
        ${WAYLAND_LIBRARIES} 
        Vulkan::Vulkan
    )
endif()

if (MSVC)
    set(RGE_COMPILER_FLAGS /W4 /permissive-)
else()
    set(RGE_COMPILER_FLAGS -Wall -Wextra -Wpedantic -Werror=return-type -Werror=uninitialized -Werror=nonnull -Werror=null-dereference -Werror=strict-aliasing -Werror=implicit-function-declaration -Werror=undef -Werror=missing-declarations -Werror=sign-compare)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE RocketRuntime_DEBUG=1) # Legacy definition
    target_compile_definitions(${PROJECT_NAME} PRIVATE ROCKETGE__DEBUG_BUILD)
    target_compile_options(${PROJECT_NAME} PRIVATE -O0 -g ${RGE_COMPILER_FLAGS})
else()
    target_compile_options(${PROJECT_NAME} PRIVATE ${RGE_COMPILER_FLAGS} -O3 -flto)
    set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_link_options(${PROJECT_NAME} PRIVATE    -Wl,-z,max-page-size=16384)

if (BUILD_TESTS)
    set(TEST_NAMES
        initializer_test
        text_test
        icon_test
        io_simulation_test
        io_test
        plugin_test
        multithreaded_test
        sound_engine_test
        triangle_drawcall_test
        default_shader_test
        persistence_test
        texture_atlas_test
        argument_test
        state_reset_test
        test_generator_test
        # render_cache_test
        custom_fbo_test
    )

    if (NOT __rge_ANDROID__)
        set(TEST_NAMES ${TEST_NAMES}
            gamepad_test
            borderless_windowed_test
        )
    endif()



    if (BUILD_ASTRO)
        set(TEST_NAMES ${TEST_NAMES}
            ui_init_test
        )
    endif()

    if (BUILD_SCRIPTING)
        set(TEST_NAMES ${TEST_NAMES}
            scripting_test
        )
    endif()

    foreach(test_name IN LISTS TEST_NAMES)
        option(BUILD_TEST_${test_name} "Build Test ${test_name}" ON)
        if (BUILD_TEST_${test_name})
            if (__rge_ANDROID__)
                add_library(${test_name} SHARED tests/${test_name}.cpp)
            else()
                add_executable(${test_name} tests/${test_name}.cpp)
            endif()

            if (__rge_ANDROID__)
                set_target_properties(${test_name} PROPERTIES
                    CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/tests
                )
            else()
                set_target_properties(${test_name} PROPERTIES
                    RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/tests
                )
            endif()

            target_link_directories(${test_name} PRIVATE bin)
            target_include_directories(${test_name} PRIVATE include)
            target_include_directories(${test_name} PRIVATE src/include)

            target_link_libraries(${test_name} PRIVATE RocketRuntime)

            if (__rge_WINDOWS__)
                if (BUILD_SCRIPTING)
                    target_link_libraries(${test_name} PRIVATE Python3::Python)
                endif()
                target_link_directories(${test_name} PRIVATE
                    ${CMAKE_SOURCE_DIR}/windeps/glew-2.3.1/lib/Release/x64
                    ${CMAKE_SOURCE_DIR}/windeps/glfw/src
                    ${CMAKE_SOURCE_DIR}/windeps/miniz
                    ${CMAKE_SOURCE_DIR}/windeps/openal-soft
                    ${CMAKE_SOURCE_DIR}/windeps/SDL2/build
                    ${CMAKE_SOURCE_DIR}/windeps/angle/lib
                )
            elseif (__rge_ANDROID__)
                target_link_libraries(${test_name} PRIVATE
                    ${CMAKE_SOURCE_DIR}/androiddeps/libs/${ANDROID_ABI}/libopenal.a
                    ${CMAKE_SOURCE_DIR}/androiddeps/libs/${ANDROID_ABI}/libSDL2.a
                    ${CMAKE_SOURCE_DIR}/androiddeps/libs/${ANDROID_ABI}/libminiz.a
                    android
                    log
                    EGL
                    GLESv3
                    OpenSLES
                    native_app_glue
                )
                target_include_directories(${test_name} PRIVATE
                    ${CMAKE_SOURCE_DIR}/androiddeps/include
                )

                target_link_options(${test_name} PRIVATE
                    -Wl,--whole-archive
                    $<TARGET_FILE:native_app_glue>
                    -Wl,--no-whole-archive
                    -Wl,--export-dynamic
                    -Wl,--defsym=stdout=0
                    -Wl,--defsym=stderr=0
                    -Wl,-z,max-page-size=16384
                )
            else()
                target_link_libraries(${test_name} PRIVATE Python3::Python)
            endif()

            if (MSVC)
                target_compile_options(${PROJECT_NAME} PRIVATE
                    /W4
                    /permissive-
                )
            else()
                target_compile_options(${test_name} PRIVATE -Wall -Wextra -Wpedantic)
            endif()

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
        # ball_smasher
    )

    foreach(example_name IN LISTS EXAMPLE_NAMES)
        option(BUILD_EXAMPLE_${example_name} "Build Example ${example_name}" ON)
        if (BUILD_EXAMPLE_${example_name})
            if (__rge_ANDROID__)
                add_library(${example_name} SHARED examples/${example_name}.cpp)
            else()
                add_executable(${example_name} examples/${example_name}.cpp)
            endif()

            if (__rge_ANDROID__)
                set_target_properties(${example_name} PROPERTIES
                    CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/examples
                )
            else()
                set_target_properties(${example_name} PROPERTIES
                    RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/examples
                )
            endif()

            target_link_directories(${example_name} PRIVATE bin)
            target_include_directories(${example_name} PRIVATE include)

            target_link_libraries(${example_name} PRIVATE RocketRuntime)

            if (__rge_WINDOWS__)
                target_link_directories(${example_name} PRIVATE
                    ${CMAKE_SOURCE_DIR}/windeps/glew-2.3.1/lib/Release/x64
                    ${CMAKE_SOURCE_DIR}/windeps/glfw/src
                    ${CMAKE_SOURCE_DIR}/windeps/miniz
                    ${CMAKE_SOURCE_DIR}/windeps/openal-soft
                    ${CMAKE_SOURCE_DIR}/windeps/SDL2/build
                    ${CMAKE_SOURCE_DIR}/windeps/angle/lib
                )
            elseif (__rge_ANDROID__)
                target_link_libraries(${example_name} PRIVATE
                    ${CMAKE_SOURCE_DIR}/androiddeps/libs/${ANDROID_ABI}/libopenal.a
                    ${CMAKE_SOURCE_DIR}/androiddeps/libs/${ANDROID_ABI}/libSDL2.a
                    ${CMAKE_SOURCE_DIR}/androiddeps/libs/${ANDROID_ABI}/libminiz.a
                    android
                    log
                    EGL
                    GLESv3
                    OpenSLES
                    native_app_glue
                )
                target_include_directories(${example_name} PRIVATE
                    ${CMAKE_SOURCE_DIR}/androiddeps/include
                )

                target_link_options(${example_name} PRIVATE
                    -Wl,--whole-archive
                    $<TARGET_FILE:native_app_glue>
                    -Wl,--no-whole-archive
                    -Wl,--export-dynamic
                    -Wl,--defsym=stdout=0
                    -Wl,--defsym=stderr=0
                    -Wl,-z,max-page-size=16384
                )

            endif()

            if (MSVC)
                target_compile_options(${PROJECT_NAME} PRIVATE
                    /W4
                    /permissive-
                )
            else()
                target_compile_options(${example_name} PRIVATE -Wall -Wextra -Wpedantic -O3)
            endif()

            set_target_properties(${example_name} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
        else()
            message("Skip example: ${example_name}")
        endif()
    endforeach()
endif()

if (BUILD_EDITOR)
    find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL Svg)

    add_executable(RGEditor 
        src/editor/main.cpp
        src/editor/code_generator.cpp
    )
    target_link_directories(RGEditor PRIVATE ${CMAKE_SOURCE_DIR}/bin)
    target_include_directories(RGEditor PRIVATE src/include include)
    target_link_libraries(RGEditor PRIVATE RocketRuntime Qt6::Widgets Qt6OpenGL Qt6OpenGLWidgets Qt6::Svg)

    set_target_properties(RGEditor PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
    )
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
message("Build Astro: ${BUILD_ASTRO}")
message("Build Scripting: ${BUILD_SCRIPTING}")
message("Build Editor: ${BUILD_EDITOR}")
