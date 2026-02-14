#ifndef ROCKETGE__RUNTIME_HPP
#define ROCKETGE__RUNTIME_HPP

#include "rocket/macros.hpp"
#ifdef ROCKETGE__Platform_Windows
#define GL_STATIC_DRAW 0x88E4
#endif

#include "renderer.hpp"
#include "asset.hpp"
#include "io.hpp"
#include "types.hpp"
#include "window.hpp"
#include "shader.hpp"

#define ROCKETGE__MAJOR_VERSION  2
#define ROCKETGE__MINOR_VERSION  0
#define ROCKETGE__BUILD          "rc"
#define ROCKETGE__STRX_(x)       #x
#define ROCKETGE__STRX(x)        ROCKETGE__STRX_(x)
#define ROCKETGE__VERSION        ROCKETGE__STRX(ROCKETGE__MAJOR_VERSION) "." ROCKETGE__STRX(ROCKETGE__MINOR_VERSION) "-" ROCKETGE__BUILD

#define ROCKETGE__FEATURE_MAX_GL_VERSION_MAJOR 4
#define ROCKETGE__FEATURE_MAX_GL_VERSION_MINOR 6


/// @brief The minimum OpenGL version required for
///     operation
/// @note Versions less than 3.0 do not operate
///         stably and may result in crashes
///         and/or unintended behavior
#define ROCKETGE__FEATURE_MIN_GL_VERSION_MAJOR 2
#define ROCKETGE__FEATURE_MIN_GL_VERSION_MINOR 0

/// @brief The max RLSL version supported
///                ^^^^ -> RocketShaderLanguage
#define ROCKETGE__FEATURE_MAX_RLSL_VERSION_MAJOR 1
#define ROCKETGE__FEATURE_MAX_RLSL_VERSION_MINOR 1

#define ROCKETGE__FEATURE_GL_LOADER "GLFNLDR"
// @brief The CPLWindowingAPI::Desktop/MobilePhone/Embedded
//            ^^^ Cross-Platform Windowing API
//              is fixed to GLFW as of now...
#define ROCKETGE__PLATFORM "GLFW::Desktop"

#define ROCKETGE__FEATURE_SHADER_SUPPORT_VERT_FRAG
// #define ROCKETGE__FEATURE_SHADER_SUPPORT_COMPUTE_SHADER

// #define ROCKETGE__NOT_IMPLEMENTED __attribute__((unavailable("Not Implemented")))
#define ROCKETGE__NOT_IMPLEMENTED
//  #define ROCKETGE__DEPRECATED [[deprecated]]
#define ROCKETGE__DEPRECATED

#include "constants.hpp"

namespace rocket {
    /// @brief Log Level
    enum class log_level_t : int {
        all = 0,
        trace = 1,
        debug = 2,
        info = 3,
        warn = 4,
        error = 5,
        fatal = 6,
        none = 100
    };

    /// @brief Sets minimum log level to be printed to the console
    /// @brief or callback
    void set_log_level(log_level_t level);

    /// @brief Converts log level to string
    /// @note lowercase
    std::string log_level_to_str(log_level_t level);

    /// @brief Log Callback
    /// @param std::string log_message
    /// @param std::string class, file, or library source
    /// @param std::string function_source
    /// @param std::string level
    using log_callback_t = std::function<void(
            std::string log_message, 
            std::string class_file_library_source, 
            std::string function_source,
            std::string level)>;

    /// @brief OpenGL::ContextVerifier Error Callback
    /// @param std::string type
    /// @param std::string severity
    /// @param int id
    /// @param std::string message
    /// @param std::string source
    using gl_error_callback_t = std::function<void(
            std::string type,
            std::string severity,
            int id,
            std::string message,
            std::string source
    )>;

    /// @brief Exit Callback
    /// @note You can choose not to exit at all
    /// @param int status_code
    using exit_callback_t = std::function<void(
            int status_code
    )>;

    /// @brief Sets log callback
    void set_log_callback(log_callback_t);

    /// @brief Set exit callback
    void set_exit_callback(exit_callback_t);

    /// @brief Log using RocketLogger or callback
    void log(std::string log, std::string class_file_library_source, std::string function_source, std::string level);

    /// @brief Exit using RocketExit or callback
    void exit(int status_code = 1);

    /// @brief Set OpenGL Error callback
    void set_opengl_error_callback(gl_error_callback_t);

    /// @brief Get OpenGL Error callback
    gl_error_callback_t get_opengl_error_callback();

    void init(std::vector<std::string> args = {});
    void init(int argc = 0, char **argv = nullptr);
}

#endif
