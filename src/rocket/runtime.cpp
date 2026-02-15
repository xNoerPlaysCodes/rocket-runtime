#include <crashdump.hpp>
#include <cstring>
#include <fstream>
#include <limits>
#include <native.hpp>
#include <rocket/runtime.hpp>
#include "util.hpp"
#include <iostream>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <rocket/macros.hpp>

namespace callback {
    void bad_memory_access(void *mem_addr, int) {
        std::endl(std::cout);

        std::cout << rocket::crash_signal(true, mem_addr, "bad_memory_access", "Invalid Memory Access");

        std::endl(std::cout);

        rnative::exit_now(1);
    }

    void invalid_memory_operation(void *mem_addr, int) {
        std::endl(std::cout);

        std::cout << rocket::crash_signal(true, mem_addr, "invalid_memory_operation", "Invalid Operation on Memory Buffer");

        std::endl(std::cout);
        
        rnative::exit_now(1);
    }

    void aborted(void *mem_addr, int) {
        std::endl(std::cout);

        std::cout << rocket::crash_signal(true, mem_addr, "aborted", "Unhandled Exception (or std::abort)");

        std::endl(std::cout);

        rnative::exit_now(1);
    }

    void __windows_crash_handler(void *mem_addr) {
        std::endl(std::cout);

        std::cout << rocket::crash_signal(true, mem_addr, "crashed", "Generic Windows Crash");

        std::endl(std::cout);

        rnative::exit_now(1);
    }
}

#ifdef ROCKETGE__Platform_UnixCompatible

#include <csignal>
#include <cstdlib>
#include <unistd.h>

void __init() {
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = [](int sig, siginfo_t *info, void *ctx) {
        callback::bad_memory_access(info->si_addr, info->si_code);
    };
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, nullptr);

    struct sigaction sbus;
    sbus.sa_sigaction = [](int sig, siginfo_t *info, void *ctx) {
        callback::invalid_memory_operation(info->si_addr, info->si_code);
    };
    sbus.sa_flags = SA_SIGINFO;
    sigaction(SIGBUS, &sbus, nullptr);

    struct sigaction sigiot;
    sigiot.sa_sigaction = [](int sig, siginfo_t *info, void *ctx) {
        callback::aborted(info->si_addr, info->si_code);
    };
    sigiot.sa_flags = SA_SIGINFO;
    sigaction(SIGIOT, &sigiot, nullptr);
    sigaction(SIGABRT, &sigiot, nullptr);

    rocket::log("Hooked SIGBUS, SIGSEGV, SIGIOT, SIGABRT", "rocket", "init", "debug");
    
    util::init_memory_buffer();

    rocket::log("Emergency memory buffer initialized with size " + std::format("{} MiB", util::get_memory_buffer()->sz / 1024 / 1024), "rocket", "init", "debug");
}

#else

#include <windows.h>

static volatile bool g_shutdown_requested = false;

BOOL WINAPI console_ctrl_handler(DWORD type)
{
    if (type == CTRL_C_EVENT ||
        type == CTRL_BREAK_EVENT ||
        type == CTRL_CLOSE_EVENT)
    {
        g_shutdown_requested = true;
        rnative::exit_now(0);
        return TRUE;
    }

    return FALSE;
}

LONG CALLBACK crash_handler(EXCEPTION_POINTERS *info) {
#ifdef _M_X64
    void* ip = (void*)info->ContextRecord->Rip;
#elif _M_IX86
    void* ip = (void*)info->ContextRecord->Eip;
#else
    void* ip = nullptr;
#endif

    if (g_shutdown_requested)
        return EXCEPTION_CONTINUE_SEARCH;

    callback::__windows_crash_handler(ip);

    return EXCEPTION_CONTINUE_SEARCH;
}

void __init() {
    // AddVectoredExceptionHandler(1, &crash_handler);
    // SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
    rocket::log("windows_platform initialization not implemented", "rocket", "init", "fixme");
    util::init_memory_buffer();

    rocket::log("Emergency memory buffer initialized with size " + std::format("{} MiB", util::get_memory_buffer()->sz / 1024 / 1024), "rocket", "init", "debug");
}

#endif

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
            case log_level_t::error:
                return "error";
            case log_level_t::fatal:
                return "fatal";
            default:
                return "unknown";
        }
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

    std::mutex cerr_mutex;
    std::ofstream std_outstm;

    std::mutex cout_mutex;
    std::ofstream std_errstm;

    bool log_to_stdouterr = false;

    void log(const std::string &log, const std::string &class_file_library_source, const std::string &function_source, const std::string &level) {
        static const auto cli_args = util::get_clistate();
        if (cli_args.lognone) return;
        std::ofstream *out = &std_outstm;
        std::mutex *mtx = &cout_mutex;
        if (level == "error" || level == "warn" || level == "fatal" || level == "fixme") {
            out = &std_errstm;
            mtx = &cerr_mutex;
        }
        {
            std::lock_guard<std::mutex> _(*mtx);
            *out << util::format_log(log, class_file_library_source, function_source, level);
            if (!log_to_stdouterr)
                out->flush();
        }
    }

    void logger_flush() {
        std::lock_guard<std::mutex> _1(cout_mutex);
        std::lock_guard<std::mutex> _2(cerr_mutex);

        std_outstm.flush();
        std_errstm.flush();
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

            std::string prefix;

            if (arg.starts_with("--")) {
                prefix = "--";
                arg = arg.substr(2);
                too_many_prefixes = argument_detected;
                argument_detected = true;
            }
            if (arg.starts_with("-")) {
                prefix = "-";
                arg = arg.substr(1);
                too_many_prefixes = argument_detected;
                argument_detected = true;
            }
            if (arg.starts_with("/")) {
                prefix = "/";
                arg = arg.substr(1);
                too_many_prefixes = argument_detected;
                argument_detected = true;
            }

            if (too_many_prefixes) {
                exit = true;
                error = true;
                rocket::log("unexpected argument: " + rawarg, "rocket", "argparse", "error");
            }

            if (value.empty() && std::find(args_with_values.begin(), args_with_values.end(), arg) != args_with_values.end()) {
                rocket::log("argument " + arg + " does not have a value where it is required", "rocket", "argparse", "error");
                continue;
            }

            if (!value.empty()) {
                ++i;
            }

            if (arg == "" && prefix == "--") {
                break;
            }

            if (arg == "noplugins" || arg == "no-plugins") {
                args.noplugins = true;
            } else if (arg == "logall" || arg == "log-all") {
                args.logall = true;
            } else if (arg == "lognone" || arg == "log-none") {
                args.lognone = true;
            } else if (arg == "debugoverlay" || arg == "doverlay" || arg == "debug-overlay") {
                args.debugoverlay = true;
            } else if (arg == "glversion" || arg == "gl-version") {
                auto res = std::from_chars(value.data(), value.data() + value.size(), args.glversion);
                if (res.ec == std::errc::invalid_argument) {
                    rocket::log("invalid value for argument: " + arg, "rocket", "argparse", "error");
                    args.glversion = GL_VERSION_UNK;
                } else {
                    if (!util::validate_gl_version_string(value)) {
                        rocket::log("invalid gl version: " + value, "rocket", "argparse", "error");
                        args.glversion = GL_VERSION_UNK;
                    }
                }
            } else if (arg == "nosplash" || arg == "no-splash") {
                args.nosplash = true;
            } else if (arg == "vp-size" || arg == "vpsize" || arg == "viewport-size" || arg == "viewportsize") {
                args.viewport_size = value;
                args.viewport_size_set = true;
            } else if (arg == "framerate") {
                if (value == "unlimited" || value == "nolimit" || value == "infinite" || value == "inf") {
                    args.framerate = std::numeric_limits<int>::max();
                } else {
                    auto res = std::from_chars(value.data(), value.data() + value.size(), args.framerate);
                    if (res.ec == std::errc::invalid_argument) {
                        rocket::log("invalid value for argument: " + arg, "rocket", "argparse", "error");
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
                    "Attribution:",
                    "> stb_vorbis    (PD)",
                    "> stb_truetype  (PD)",
                    "> stb_image     (PD)",
                    "> tweeny       (MIT)",
#ifdef ROCKETGE__GLFNLDR_BACKEND_GLEW
                    "> GLEW         (BSD)",
#endif
#ifdef ROCKETGE__GLFNLDR_BACKEND_LIBEPOXY
                    "> libepoxy     (MIT)",
#endif
                    "> OpenGL      (RYLF)",
                    "> glm          (MIT)",
                    "> GLFW        (ZLIB)",
                    "> SDL2        (ZLIB)",
                    "> OpenAL Soft (LGPL)",
                    "> miniz        (MIT)",
                    "> nlohmannJSON (MIT)",
                    "> pybind11     (BSD)",
                    "> python3      (PSF)",
                    "",
#ifdef ROCKETGE__Platform_UnixCompatible
                    "Made by noerlol with  ",
#else
                    "Made by noerlol with love",
#endif
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
                    "Format: [--arg] or [-arg] or [/arg]",
                    "",
                    "Arguments:",
                    "   no-plugins, noplugins",
                    "   -> disable all plugins before startup",
                    "",
                    "   log-all, logall / log-none, lognone",
                    "   -> logs every / no message(s)",
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
                    "*  viewport-size, viewportsize, vp-size, vpsize [resolution] [[FIXME]]",
                    "   -> forces to use a initial viewport size",
                    "",
                    "*  framerate [fps]",
                    "   -> forces to use a set framerate (if reachable)",
                    "",
                    "   version",
                    "   -> shows version and attribution",
                    "",
                    "* | Value Mandatory",
                    "",
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
            else if (arg == "force-wayland" || arg == "forcewayland") {
                args.forcewayland = true;
            }
            else if (arg == "build-info" || arg == "buildinfo") {
#if defined(__clang__)
                const std::string compiler_name = "Clang";
#elif defined(__GNUC__) || defined(__GNUG__)
                const std::string compiler_name = "GCC";
#elif defined(_MSC_VER)
                const std::string compiler_name = "MSVC";
#else
                const std::string compiler_name = "Unknown";
#endif

                const std::vector<std::string> lines = {
                    "RocketGE " ROCKETGE__VERSION,
                    "Compiled with " + compiler_name,
                };

                for (auto &l : lines) {
                    std::cout << l << '\n';
                }

                exit = true;
            }
            else {
                rocket::log("unexpected argument: " + arg + (value.empty() ? "" : " with value: " + value), "rocket", "argparse", "error");
                exit = true;
                error = true;
            }
        }

        if (error && exit) {
            rocket::log("one or more errors found in parser", "rocket", "argparse", "fatal");
            if (!additional_exit_message.empty()) {
                rocket::log(additional_exit_message, "rocket", "argparse", "fatal");
            }
            rocket::exit(1);
        }

        if (exit) {
            rocket::exit(0);
        }

        util::init_clistate(args);
    }

    void set_cli_arguments(std::vector<std::string> args) {
        std::vector<char *> strbuf;
        strbuf.reserve(args.size());
        for (auto &a : args) strbuf.push_back(a.data());
        set_cli_arguments(args.size(), strbuf.data());
    }

    void global_init() {
        std::lock_guard<std::mutex> _1(cout_mutex);
        std::lock_guard<std::mutex> _2(cerr_mutex);

        std_outstm.open(rocket::cst::std_out);
        std_errstm.open(rocket::cst::std_err);
    }

    void init(std::vector<std::string> args) {
        set_cli_arguments(args);
        rnative::init();
        __init();
        global_init();
    }

    void init(int argc, char **argv) {
        set_cli_arguments(argc, argv);
        rnative::init();
        __init();
        global_init();
    }

    void set_logger_file_output(const std::filesystem::path &path) {
        std::lock_guard<std::mutex> _1(cout_mutex);
        std::lock_guard<std::mutex> _2(cerr_mutex);

        log_to_stdouterr = (path == rocket::cst::std_out || path == rocket::cst::std_err);

        std_outstm.open(path);
        std_errstm.open(path);
    }
}
