#include "../../include/rocket/runtime.hpp"
#include "util.hpp"
#include <iostream>

namespace rocket {
    void set_log_level(log_level_t level) { util::set_log_level(level); }

    gl_error_callback_t glerror_cb_default = [](std::string, std::string, int, std::string, std::string) {};

    gl_error_callback_t glerror_cb = nullptr;
    exit_callback_t exitcb = nullptr;

    std::string log_level_to_str(log_level_t level) {
        switch (level) {
            case log_level_t::trace:
                return "trace";
            case log_level_t::debug:
                return "debug";
            case log_level_t::info:
                return "info";
            case log_level_t::warn:
                return "warn";
            case log_level_t::fatal_to_function:
                return "fatal_to_function";
            case log_level_t::fatal:
                return "fatal";
            default:
                return "unknown";
        }
    }

    void set_log_error_callback(log_error_callback_t callback) { 
        util::set_log_error_callback(callback); 
    }
    void set_log_callback(log_callback_t callback) { 
        util::set_log_callback(callback); 
    }

    void log_error(std::string error, int error_id, std::string error_source, std::string level) { 
        std::cerr << util::format_error(error, error_id, error_source, level);
    }

    void log(std::string log, std::string class_file_library_source, std::string function_source, std::string level) {
        std::cout << util::format_log(log, class_file_library_source, function_source, level);
    }

    void set_opengl_error_callback(gl_error_callback_t cb) {
        glerror_cb = cb;
    }

    void set_exit_callback(exit_callback_t cb) {
        exitcb = cb;
    }

    void exit(int status_code) {
        if (exitcb != nullptr) {
            exitcb(status_code);
        }

        std::exit(status_code);
    }

    gl_error_callback_t get_opengl_error_callback() {
        return glerror_cb == nullptr ? glerror_cb_default : glerror_cb;
    }
}
