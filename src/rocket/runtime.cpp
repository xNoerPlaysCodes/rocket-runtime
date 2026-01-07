#include "../../include/rocket/runtime.hpp"
#include "util.hpp"
#include <iostream>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <rocket/macros.hpp>

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

    void log_error(std::string error, std::string error_source, std::string level) {
        log_error(error, -1, error_source, level);
    }

    void __log_error_with_id(std::string error, int error_id, std::string error_source, std::string level) {
        log_error(error, error_id, error_source, level);
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
        } else {
            std::exit(status_code);
        }
    }

    gl_error_callback_t get_opengl_error_callback() {
        return glerror_cb == nullptr ? glerror_cb_default : glerror_cb;
    }

    void set_cli_arguments(int argc, char *argv[]) {
        if (argc < 0) {
            return;
        }

        const std::vector<std::string> args_with_values = {
            "viewport-size", 
            "viewportsize", 
            "vp-size", 
            "vpsize",
            "framerate",
        };
        util::global_state_cliargs_t args = {};
        bool exit = false;
        bool error = false;
        std::string additional_exit_message;
        for (int i = 1; i < argc; ++i) {
            int nexti = i + 1;
            std::string arg = argv[i];
            std::string value = "";
            if (nexti != argc) {
                if (argv[nexti][0] != '-' && argv[nexti][0] != '/') {
                    value = argv[nexti];
                }
            }

            std::string rawarg = arg;

            bool argument_detected = false;
            bool too_many_prefixes;

            if (arg.starts_with("--")) {
                arg = arg.substr(2);
                too_many_prefixes = argument_detected;
                argument_detected = true;
            }
            if (arg.starts_with("-")) {
                arg = arg.substr(1);
                too_many_prefixes = argument_detected;
                argument_detected = true;
            }
            if (arg.starts_with("/")) {
                arg = arg.substr(1);
                too_many_prefixes = argument_detected;
                argument_detected = true;
            }

            if (too_many_prefixes) {
                exit = true;
                error = true;
                rocket::log_error("unexpected argument: " + rawarg, -1, "rocket::argparse", "error");
            }

            if (value.empty() && std::find(args_with_values.begin(), args_with_values.end(), arg) != args_with_values.end()) {
                rocket::log_error("argument " + arg + " does not have a value where it is required", -1, "rocket::argparse", "error");
                continue;
            }

            if (!value.empty()) {
                ++i;
            }

            if (arg == "noplugins" || arg == "no-plugins") {
                args.noplugins = true;
            } else if (arg == "logall" || arg == "log-all") {
                args.logall = true;
            } else if (arg == "debugoverlay" || arg == "doverlay" || arg == "debug-overlay") {
                args.debugoverlay = true;
            } else if (arg == "glversion" || arg == "gl-version") {
                auto res = std::from_chars(value.data(), value.data() + value.size(), args.glversion);
                if (res.ec == std::errc::invalid_argument) {
                    rocket::log_error("invalid value for argument: " + arg, "rocket::argparse", "error");
                    args.glversion = GL_VERSION_UNK;
                } else {
                    if (!util::validate_gl_version_string(value)) {
                        rocket::log_error("invalid gl version: " + value, "rocket::argparse", "error");
                        args.glversion = GL_VERSION_UNK;
                    }
                }
            } else if (arg == "nosplash" || arg == "no-splash") {
                args.nosplash = true;
            } else if (arg == "dx11" || arg == "dx-11") {
                rocket::log_error("argument handler not implemented for " + arg, "rocket::argparse", "error");
                args.dx11 = true;
            } else if (arg == "dx12" || arg == "dx-12") {
                rocket::log_error("argument handler not implemented for " + arg, "rocket::argparse", "error");
                args.dx12 = true;
            } else if (arg == "vp-size" || arg == "vpsize" || arg == "viewport-size" || arg == "viewportsize") {
                args.viewport_size = value;
                args.viewport_size_set = true;
            } else if (arg == "framerate") {
                if (value == "unlimited" || value == "nolimit" || value == "infinite" || value == "inf") {
                    args.framerate = 2147483647;
                } else {
                    auto res = std::from_chars(value.data(), value.data() + value.size(), args.framerate);
                    if (res.ec == std::errc::invalid_argument) {
                        rocket::log_error("invalid value for argument: " + arg, "rocket::argparse", "error");
                        args.framerate = -1;
                    }
                }
            } else if (arg == "version") {
                exit = true;
                std::vector<std::string> lines = {
                    "RocketGE (rge) " ROCKETGE__VERSION,
                    "Copyright (C) 2026 noerlol",
                    "License MIT: Refer to LICENSE for more",
                    "",
                    "Additional Technologies used:",
                    "> stb_vorbis  (PD)",
                    "> stb_trutype (PD)",
                    "> stb_image   (PD)",
                    "> tweeny      (MIT)",
                    "",
                    "Made by noerlol with  ",
                };
                for (auto &l : lines) {
                    std::cout << l << '\n';
                }
            }
            else if (arg == "help") {
                exit = true;

                std::vector<std::string> lines = {
                    "Usage: " + std::string(argv[0]) + " [options]",
                    "",
                    "Arguments may start with java-style (-), GNU-long-style (--) or windows-style (/) on any platform\n",
                    "Arguments:",
                    "   no-plugins, noplugins",
                    "   -> disable all plugins before startup",
                    "",
                    "   log-all, logall",
                    "   -> logs every message",
                    "",
                    "   debug-overlay, debugoverlay, doverlay",
                    "   -> shows a debug overlay with rendering information",
                    "",
                    "*  gl-version, glversion [2.0 -> 4.6]",
                    "   -> forces an OpenGL version to be used",
                    "",
                    "   no-splash, nosplash",
                    "   -> hides splash from being shown in the beginning",
                    "",
#ifdef ROCKETGE__Platform_Windows
                    "W  dx-11, dx11 [[FIXME]]",
                    "   -> forces to use D3D11 renderer",
                    "",
                    "W  dx-12, dx12 [[FIXME]]",
                    "   -> forces to use D3D12 renderer",
                    "",
#endif
                    "*  viewport-size, viewportsize, vp-size, vpsize [resolution] [[FIXME]]",
                    "   -> forces to use a initial viewport size",
                    "",
                    "*  framerate [fps]",
                    "   -> forces to use a set framerate (if reachable)",
                    "",
                    "   version",
                    "   -> shows version and attribution",
                    "",
                    "Values to arguments marked with * are mandatory",
                    "",
                    "--> Made with RocketGE version " + std::string(ROCKETGE__VERSION),
                    "--> Powered by RocketGE"
                };

                for (auto &line : lines) {
                    std::cout << line << '\n';
                }
            }
            // -- undocumented (do not add to help menu) --
            else if (arg == "no-text" || arg == "notext") {
                args.notext = true;
            }
            else {
                rocket::log_error("unexpected argument: " + arg + (value.empty() ? "" : " with value: " + value), -1, "rocket::argparse", "error");
                exit = true;
                error = true;
            }
        }

        if (error && exit) {
            rocket::log_error("one or more errors found in parser", -1, "rocket::argparse", "fatal");
            if (!additional_exit_message.empty()) {
                rocket::log_error(additional_exit_message, -1, "rocket::argparse", "fatal");
            }
            rocket::exit(1);
        }

        if (exit) {
            rocket::exit(1);
        }

        util::init_clistate(args);
    }
}
