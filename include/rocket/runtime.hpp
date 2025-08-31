#ifndef ROCKETGE__RUNTIME_HPP
#define ROCKETGE__RUNTIME_HPP

#include "renderer.hpp"
#include "asset.hpp"
#include "io.hpp"
#include "types.hpp"
#include "window.hpp"

#define ROCKETGE__MINOR_VERSION  1
#define ROCKETGE__MAJOR_VERSION  0
#define ROCKETGE__BUILD          "open-dev"
#define ROCKETGE__VERSION        #ROCKETGE_MAJOR_VERSION "." #ROCKETGE_MINOR_VERSION "-" ROCKETGE_BUILD

#define ROCKETGE__FEATURE_MAX_GL_VERSION_MAJOR 4
#define ROCKETGE__FEATURE_MAX_GL_VERSION_MINOR 6

#define ROCKETGE__FEATURE_MIN_GL_VERSION_MAJOR 3
#define ROCKETGE__FEATURE_MIN_GL_VERSION_MINOR 0

#define ROCKETGE__FEATURE_GL_LOADER "GLEW"
#define ROCKETGE__PLATFORM "GLFW::Desktop"

#define ROCKETGE__FEATURE_SHADER_SUPPORT_VERT_FRAG
// #define ROCKETGE__FEATURE_SHADER_SUPPORT_COMPUTE_SHADER

namespace rocket {
    enum class log_level_t : int {
        trace = 0,
        debug = 1,
        info = 2,
        warn = 3,
        fatal_to_function = 4,
        fatal = 5,
    };

    void set_log_level(log_level_t level);
    using log_error_callback_t = std::function<void(
            std::string error, 
            int error_id, 
            std::string error_source, 
            std::string level)>;

    std::string log_level_to_str(log_level_t level);
    void set_log_error_callback(log_error_callback_t);

    using log_callback_t = std::function<void(
            std::string log, 
            std::string class_file_library_source, 
            std::string function_source,
            std::string level)>;
    void set_log_callback(log_callback_t);

    void log_error(std::string error, int error_id, std::string error_source, std::string level);
    void log(std::string log, std::string class_file_library_source, std::string function_source, std::string level);
}

#endif
