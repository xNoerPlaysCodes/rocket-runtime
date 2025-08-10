#ifndef ROCKETGE__RUNTIME_HPP
#define ROCKETGE__RUNTIME_HPP

#include "asset.hpp"
#include "io.hpp"
#include "renderer.hpp"
#include "types.hpp"
#include "window.hpp"

namespace rocket {
    #define ROCKETGE_MINOR_VERSION  "1"
    #define ROCKETGE_MAJOR_VERSION  "0"
    #define ROCKETGE_BUILD          "open-dev"
    #define ROCKETGE_VERSION        ROCKETGE_MAJOR_VERSION "." ROCKETGE_MINOR_VERSION "-" ROCKETGE_BUILD
}
#define ROCKETGE_OpenGL_SupportComputeShader

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
