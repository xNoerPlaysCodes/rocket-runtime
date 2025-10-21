#include "../../include/rocket/runtime.hpp"
#include "util.hpp"
#include <iostream>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>

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
            case log_level_t::fatal_to_function: // || error:
                return "error";
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

    std::mutex log_mutex;
    std::queue<std::string> log_queue;
    std::condition_variable log_cv;

    std::atomic<bool> log_thread_continue = false;
    std::thread log_thread;

    void log_init() {
        log_thread = std::thread([]() {
            while (log_thread_continue) {
                std::unique_lock<std::mutex> lock(log_mutex);
                log_cv.wait(lock, [] { return !log_queue.empty() || !log_thread_continue; });
                while (!log_queue.empty()) {
                    std::cout << log_queue.front();
                    log_queue.pop();
                }
            }
        });
    }

    std::mutex log_error_mutex;
    std::queue<std::string> log_error_queue;
    std::condition_variable log_error_cv;

    std::atomic<bool> log_error_thread_continue = false;
    std::thread log_error_thread;

    void log_error_init() {
        log_error_thread = std::thread([]() {
            while (log_error_thread_continue) {
                std::unique_lock<std::mutex> lock(log_error_mutex);
                log_error_cv.wait(lock, [] { return !log_error_queue.empty() || !log_error_thread_continue; });
                while (!log_error_queue.empty()) {
                    std::cerr << log_error_queue.front();
                    log_error_queue.pop();
                }
            }
        });
    }

    std::mutex cerr_mutex;
    std::mutex cout_mutex;

    void log_error(std::string error, int error_id, std::string error_source, std::string level) {
        {
            std::lock_guard<std::mutex> _(cerr_mutex);
            std::cerr << util::format_error(error, error_id, error_source, level);
        }
    }

    void log(std::string log, std::string class_file_library_source, std::string function_source, std::string level) {
        {
            std::lock_guard<std::mutex> _(cout_mutex);
            std::cout << util::format_log(log, class_file_library_source, function_source, level);
        }
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

    void set_cli_arguments(int argc, char *argv[]) {
        if (argc < 0) {
            return;
        }

        const std::vector<std::string> args_with_values = {};
        util::global_state_cliargs_t args = {};
        for (int i = 0; i < argc; ++i) {
            int nexti = i + 1;
            std::string arg = argv[i];
            std::string value = "";
            if (nexti != argc) {
                if (argv[nexti][0] != '-' || argv[nexti][0] != '/') {
                    value = argv[nexti];
                }
            }

            if (arg.starts_with("--")) {
                arg = arg.substr(2);
            }
            if (arg.starts_with("-")) {
                arg = arg.substr(1);
            }
            if (arg.starts_with("/")) {
                arg = arg.substr(1);
            }

            if (value.empty() && std::find(args_with_values.begin(), args_with_values.end(), arg) != args_with_values.end()) {
                rocket::log_error("argument " + arg + " does not have a value where it is required", -1, "RocketRuntime", "error");
                continue;
            }

            if (arg == "noplugins") {
                args.noplugins = true;
            } else if (arg == "logall") {
                args.logall = true;
            } else if (arg == "debugoverlay") {
                args.debugoverlay = true;
            } else if (arg == "glversion") {
                args.glversion = GL_VERSION_UNK; // TODO Replace
            }
        }

        util::init_clistate(args);
    }
}
