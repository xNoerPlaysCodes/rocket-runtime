#include <crashdump.hpp>
#include <fstream>
#include <limits>
#include <native.hpp>
#include <rocket/runtime.hpp>
#include "util.hpp"
#include <iostream>
#include <string>
#include <mutex>
#include <rocket/macros.hpp>
#include <rocket/glfnldr.hpp>
#include <cstdlib>
#include <intl_macros.hpp>
#include <thread>

#ifdef ROCKETGE__Platform_Android
#include <android/log.h>
#endif

namespace rocket::globals {
    std::thread::id g_main_thread_id;
    bool g_main_thread_id_set = false;

    bool g_rocket_entrypoint_used = false;
}

namespace callback {
    void bad_memory_access(void *mem_addr, int) {
#ifdef ROCKETGE__Platform_Android
        __android_log_print(ANDROID_LOG_INFO, "RocketGE", "%s", rocket::crash_signal(true, mem_addr, "bad_memory_access", "Invalid Memory Access"));
        rnative::exit_now(1);
#endif
        std::endl(std::cout);

        std::cout << rocket::crash_signal(true, mem_addr, "bad_memory_access", "Invalid Memory Access");

        std::endl(std::cout);

        rnative::exit_now(1);
    }

    void stack_overflow(void*, int) {
#ifdef ROCKETGE__Platform_Android
        __android_log_print(ANDROID_LOG_INFO, "RocketGE", "%s", "FATAL! Stack Overflow\n");
        rnative::exit_now(1);
#endif
        std::endl(std::cout);

        std::cout << "FATAL! Stack Overflow\n";

        std::endl(std::cout);

        rnative::exit_now(1);
    }

    void invalid_memory_operation(void *mem_addr, int) {
#ifdef ROCKETGE__Platform_Android
        __android_log_print(ANDROID_LOG_INFO, "RocketGE", "%s", rocket::crash_signal(true, mem_addr, "invalid_memory_operation", "Invalid Operation on Memory Buffer"));
        rnative::exit_now(1);
#endif

        std::endl(std::cout);

        std::cout << rocket::crash_signal(true, mem_addr, "invalid_memory_operation", "Invalid Operation on Memory Buffer");

        std::endl(std::cout);
        
        rnative::exit_now(1);
    }

    void aborted(void *mem_addr, int) {
#ifdef ROCKETGE__Platform_Android
        __android_log_print(ANDROID_LOG_INFO, "RocketGE", "%s", rocket::crash_signal(true, mem_addr, "aborted", "Unhandled Exception or std::terminate or std::abort"));
        rnative::exit_now(1);
#endif

        std::endl(std::cout);

        std::cout << rocket::crash_signal(true, mem_addr, "aborted", "Unhandled Exception or std::terminate or std::abort");

        std::endl(std::cout);

        rnative::exit_now(1);
    }

    void __windows_crash_handler(void *mem_addr) {
        std::endl(std::cout);

        std::cout << rocket::crash_signal(true, mem_addr, "crashed", "Generic Windows Exception");

        std::endl(std::cout);

        rnative::exit_now(1);
    }
}

#if defined(ROCKETGE__Platform_UnixCompatible) || defined(ROCKETGE__Platform_Android)

#include <csignal>
#include <cstdlib>
#include <unistd.h>

void __hook(int signal, void(*func)(int, siginfo_t *, void *)) {
    struct sigaction sa = {};
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = func;
    sigaction(signal, &sa, nullptr);
}

void __init() {
    __hook(SIGSEGV, [](int, siginfo_t *info, void *) {
        callback::bad_memory_access(info->si_addr, info->si_code);
    });

    __hook(SIGBUS, [](int, siginfo_t *info, void *) {
        callback::invalid_memory_operation(info->si_addr, info->si_code);
    });

    __hook(SIGIOT, [](int, siginfo_t *info, void *) {
        callback::aborted(info->si_addr, info->si_code);
    });

    __hook(SIGABRT, [](int, siginfo_t *info, void *) {
        callback::aborted(info->si_addr, info->si_code);
    });

    rocket::log("Hooked SIGBUS, SIGSEGV, SIGIOT, SIGABRT", "rocket", "init", "debug");
    
    util::init_memory_buffer();
    rnative::set_thread_name("RocketGE");

    rocket::log("Emergency memory buffer initialized with size " + std::format("{} MiB", util::get_memory_buffer()->sz / 1024 / 1024), "rocket", "init", "debug");
}

#elif defined(ROCKETGE__Platform_Windows) && defined(ROCKETGE__Platform_Desktop)

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

    if (g_shutdown_requested) {
        rocket::exit(0);
        return EXCEPTION_CONTINUE_SEARCH;
    }

    callback::__windows_crash_handler(ip);

    return EXCEPTION_CONTINUE_SEARCH;
}

void __init() {
    // AddVectoredExceptionHandler(1, &crash_handler);
    // SEH Handler
    SetUnhandledExceptionFilter(&crash_handler);
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
    rocket::log("Hooked SEH", "rocket", "init", "debug");
    util::init_memory_buffer();

    rocket::log("Emergency memory buffer initialized with size " + std::format("{} MiB", util::get_memory_buffer()->sz / 1024 / 1024), "rocket", "init", "debug");
}

#endif

namespace rocket {
    void set_log_level(log_level_t level) { util::set_log_level(level); }

    static gl_error_callback_t glerror_cb = [](std::string, std::string, int, std::string, std::string) {};

    static exit_callback_t exitcb = nullptr;

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

    static std::vector<logger_state_t> logger_state;
    static std::mutex logger_state_mutex;

    static std::mutex cerr_mutex;
    static std::ofstream std_outstm;

    static std::mutex cout_mutex;
    static std::ofstream std_errstm;

    static bool log_to_stdouterr = false;

    void log(const std::string &log, const std::string &class_file_library_source, const std::string &function_source, const std::string &level) {
#ifdef ROCKETGE__Platform_Android
        std::string formatted = util::format_log(log, class_file_library_source, function_source, level);
        __android_log_print(ANDROID_LOG_INFO, "RocketGE", "%s", formatted.c_str());
        return;
#else
        static std::atomic_bool cli_init = false;
        if (!cli_init && !std_outstm.is_open() && !std_errstm.is_open()) [[unlikely]] {
            cli_init = true;
            std_outstm.open(cst::std_out);
            std_errstm.open(cst::std_err);
            if (!rocket::globals::g_rocket_entrypoint_used) {
                rocket::log("rocket_main was not used, runtime features WILL be limited", "rocket", "log", "error");
            }
            rocket::log("rocket::init was not called, runtime features WILL be limited", "rocket", "log", "error");
        }
        static const auto cli_args = util::get_clistate();
        std::ofstream *out = &std_outstm;
        std::mutex *mtx = &cout_mutex;
        // if (level == "error" || level == "warn" || level == "fatal" || level == "fixme") {
        //     out = &std_errstm;
        //     mtx = &cerr_mutex;
        // }
        switch (level[0]) {
            case 'e':
            case 'w':
            case 'f':
                out = &std_errstm;
                mtx = &cerr_mutex;
        }
        {
            {
                std::lock_guard<std::mutex> _(*mtx);
                *out << util::format_log(log,class_file_library_source, function_source, level);
            }
            {
                std::lock_guard<std::mutex> _(logger_state_mutex);
                if (std::find(logger_state.begin(), logger_state.end(), logger_state_t::flush_never) != logger_state.end()) return;
                if (!log_to_stdouterr || std::find(logger_state.begin(), logger_state.end(), logger_state_t::flush_always) != logger_state.end()) {
                    out->flush();
                }
            }
        }
#endif
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
        }

        rnative::exit_now(status_code);
    }

    gl_error_callback_t get_opengl_error_callback() {
        return glerror_cb;
    }

    void global_init() {
        static bool initialized = false;
        if (initialized) return;
        initialized = true;

        std::lock_guard<std::mutex> _1(cout_mutex);
        std::lock_guard<std::mutex> _2(cerr_mutex);

        std_outstm.open(rocket::cst::std_out);
        std_errstm.open(rocket::cst::std_err);
    }

    void logger_push(logger_state_t state) {
        std::lock_guard<std::mutex> _(logger_state_mutex);
        if (std::find(logger_state.begin(), logger_state.end(), state) != logger_state.end()) return;
        logger_state.push_back(state);
    }

    void logger_pop(logger_state_t state) {
        std::lock_guard<std::mutex> _(logger_state_mutex);
        for (auto it = logger_state.begin(); it != logger_state.end(); ++it) {
            if (*it == state) {
                logger_state.erase(it);
                break;
            }
        }
    }

    struct custom_argument_t {
        std::string arg;
        std::string description;
        std::string value_type;
        bool value;
        std::function<void(std::optional<std::string>)> cb;
    };

    struct library_attribution_t {
        std::string name;
        std::string license;
    };

    std::vector<custom_argument_t> registered_arguments;
    std::vector<library_attribution_t> registered_libattributions;

    void set_cli_arguments(int argc, char *argv[]) {
        const std::vector<std::string> args_with_values = {
            "viewport-size", "viewportsize", "vp-size", "vpsize",
            "framerate",
            "logfile",
        };

        util::global_state_cliargs_t args = {};
        bool exit = false;
        bool error = false;
        bool logfile_overwrite = false;
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
            bool too_many_prefixes = false;

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
                additional_exit_message = "argument " + arg + " does not have a value where it is required";
                exit = true;
                error = true;
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
            } else if (arg == "debugoverlay" || arg == "doverlay" || arg == "debug-overlay") {
                args.debugoverlay = true;
            } else if (arg == "renderer-backend") {
                constexpr auto split = [](std::string str, char delim) -> std::vector<std::string> {
                    std::stringstream ss(str);
                    std::string token;
                    std::vector<std::string> tokens;
                    while (std::getline(ss, token, delim)) {
                        tokens.push_back(token);
                    }
                    return tokens;
                };

                constexpr auto version_to_int = [](std::string str) -> int {
                    if (str == "any")
                        return API_VERSION_ANY;
                    std::string accum;
                    for (char c : str) {
                        if (c >= '0' && c <= '9')
                            accum += c;
                    }

                    return std::stoi(accum);
                };

                std::vector<std::string> parts = split(value, ':');
                if (parts.size() != 2) {
                    rocket::log("invalid format: " + value, "rocket", "argparse", "fatal");
                    error = true;
                    exit = true;
                }

                const std::string &backend = parts.at(0);
                const std::string &version = parts.at(1);

                if (backend == "opengl") {
#ifdef ROCKETGE__Platform_Desktop
                    auto res = std::from_chars(version.data(), version.data() + version.size(), args.glversion);
                    if (res.ec == std::errc::invalid_argument) {
                        rocket::log("invalid value for argument: " + arg, "rocket", "argparse", "error");
                        args.glversion = GL_VERSION_UNK;
                    } else {
                        if (!util::validate_gl_version_string(version)) {
                            rocket::log("invalid gl version: " + value, "rocket", "argparse", "fatal");
                            error = true;
                            exit = true;
                            args.glversion = GL_VERSION_UNK;
                        }
                    }
#else
                    args.glversion = std::stof(std::string(value));
#endif
                } else if (backend == "vulkan") {
                    args.renderer_backend = renderer_backend_t::vulkan;
                } else {
                    rocket::log("invalid renderer backend: " + backend, "rocket", "argparse", "fatal");
                }

                args.renderer_backend_version = version_to_int(version);
            } else if (arg == "nosplash" || arg == "no-splash") {
                args.nosplash = true;
            } else if (arg == "vp-size" || arg == "vpsize" || arg == "viewport-size" || arg == "viewportsize") {
                args.viewport_size = value;
                args.viewport_size_set = true;
            } else if (arg == "framerate") {
                if (value == "unlimited" || value == "nolimit" || value == "infinite" || value == "inf") {
                    args.framerate = std::numeric_limits<int>::max();
                } else {
#ifdef ROCKETGE__Platform_Desktop
                    auto res = std::from_chars(value.data(), value.data() + value.size(), args.framerate);
                    if (res.ec == std::errc::invalid_argument) {
                        rocket::log("invalid value for argument: " + arg, "rocket", "argparse", "error");
                        args.framerate = -1;
                        exit = true;
                        error = true;
                    }
#else
                    args.framerate = std::stoi(std::string(value.data(), value.size()));
#endif
                }
            } else if (arg == "logfile") {
                if (std::filesystem::exists(value) && !logfile_overwrite) {
                    additional_exit_message = "file already exists, cannot overwrite without logfile-overwrite";
                    exit = true;
                    error = true;
                } else {
                    if (value == "stdnull" || value == "devnull" || value == "discard" || value == "NUL") {
                        set_logger_file_output(rocket::cst::stdnull);
                    } else {
                        if (value.starts_with('!')) value = value.substr(1);
                        set_logger_file_output(value);
                        {
                            std::lock_guard<std::mutex> _1(cout_mutex);
                            std::lock_guard<std::mutex> _2(cerr_mutex);
                            
                            std_outstm << "--- RocketGE Logger Output to File ---\n";
                            std_outstm.flush();
                        }
                    }
                }
            } else if (arg == "logfile-overwrite") {
                logfile_overwrite = true;
            }
            else if (arg == "version") {
                exit = true;
                std::vector<std::string> lines = {
                    "RocketGE v" ROCKETGE__VERSION,
                    "Copyright (C) 2026 noerlol",
                    "License: MIT, Refer to LICENSE for more",
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
                    "> OpenGL       (SGI)",
                    "> Vulkan    (APACHE)",
                    "> glm          (MIT)",
                    "> GLFW        (ZLIB)",
                    "> SDL2        (ZLIB)",
                    "> OpenAL Soft (LGPL)",
                    "> miniz        (MIT)",
                    "> nlohmannJSON (MIT)",
                    "> pybind11     (BSD)",
                    "> python3      (PSF)",
                    "> lzf          (BSD)",
                    "",
                };
                if (registered_libattributions.size() > 0)
                    lines.push_back("Game Specific:");
                for (const auto &libattribution : registered_libattributions) {
                    lines.push_back("> " + libattribution.name + ": " + libattribution.license);
                }
#ifdef ROCKETGE__Platform_UnixCompatible
                    lines.push_back("Made by noerlol with  ");
#else
                    lines.push_back("Made by noerlol with love");
#endif
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
                    "   log-all, logall",
                    "   -> logs every / no message(s)",
                    "",
                    "*  logfile [file_path] (!path if path starts with /)",
                    "   -> logs messages",
                    "",
                    "   debug-overlay, debugoverlay, doverlay",
                    "   -> shows a debug overlay with rendering information",
                    "",
                    "*  renderer-backend, [name:version] (version fmt: Major.Minor OR any)",
                    "   -> forces a specific renderer backend to be used (if available)",
                    "",
                    "   no-splash, nosplash",
                    "   -> hides splash from being shown in the beginning",
                    "",
                    "*  viewport-size, viewportsize, vp-size, vpsize [width]x[height]",
                    "   -> forces to use a initial viewport (and window) size",
                    "",
                    "*  framerate [fps]",
                    "   -> forces to use a set framerate (if reachable)",
                    "",
                    "   version",
                    "   -> shows version and attribution",
                    "",
                };

                if (registered_arguments.size() > 0) {
                    lines.push_back("Game Specific:");
                }

                for (auto &a : registered_arguments) {
                    if (a.value)
                        lines.push_back("*  " + a.arg + " [" + a.value_type + "]");
                    else
                        lines.push_back("   " + a.arg);
                    lines.push_back("   -> " + a.description);
                    lines.push_back("");
                }

                lines.push_back("* | Value Mandatory");
                lines.push_back("");
                lines.push_back("--> Powered by RocketGE");

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

                std::vector<glfnldr::backend_t> backends = glfnldr::get_backends();
                std::string backends_str;
                for (size_t i = 0; i < backends.size(); ++i) {
                    std::string str;
                    if (backends[i] == glfnldr::backend_t::null) {
                        str = "null";
                    } else if (backends[i] == glfnldr::backend_t::glad) {
                        str = "glad";
                    } else if (backends[i] == glfnldr::backend_t::glew) {
                        str = "glew";
                    }
                    backends_str += str + " ";
                }
                const std::vector<std::string> lines = {
                    "RocketGE " ROCKETGE__VERSION,
                    "Compiled with " + compiler_name,
                    "GLfnldr Backends: " + backends_str
                };

                for (auto &l : lines) {
                    std::cout << l << '\n';
                }

                exit = true;
            } else if (arg == "software-frame-timer") {
                args.software_frame_timer = true;
            } else if (arg == "mesa-software") {
#ifdef ROCKETGE__Platform_Linux
                setenv("LIBGL_ALWAYS_SOFTWARE", "1", 1);
#endif
            } else if (arg == "intentional-crash") {
                util::segfault();
            }
            else {
                bool found = false;
                for (auto &a : registered_arguments) {
                    if (a.arg != arg) {
                        continue;
                    }

                    if (a.value && value.empty()) {
                        additional_exit_message = "argument " + arg + " does not have a value where it is required";
                        exit = true;
                        error = true;
                        continue;
                    }

                    found = true;

                    if (a.value) {
                        a.cb(value);
                    } else {
                        a.cb(std::nullopt);
                    }
                }

                if (!found) {
                    rocket::log("unexpected argument: " + (arg.empty() ? "(empty)" : arg) + (value.empty() ? "" : " with value: " + value), "rocket", "argparse", "error");
                    exit = true;
                    error = true;
                }
            }
        }

        if (error && exit) {
            rocket::log("one or more errors found in parser", "rocket", "argparse", "fatal");
            if (!(additional_exit_message.empty())) {
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

    void init(std::vector<std::string> args) {
#ifdef ROCKETGE__Platform_Android
        __init();
        global_init();
        rnative::init();
#else
        global_init();
        rnative::init();
        __init();
#endif

        set_cli_arguments(args);
    }
    

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winfinite-recursion"

    void __trigger_stack_overflow() {
        __trigger_stack_overflow();
    }

#pragma GCC diagnostic pop

    void init(int argc, char **argv) {
#ifdef ROCKETGE__Platform_Android
        __init();
        global_init();
        rnative::init();
#else
        global_init();
        rnative::init();
        __init();
#endif

        set_cli_arguments(argc, argv);
    }

    void set_logger_file_output(const std::filesystem::path &path) {
        std::lock_guard<std::mutex> _1(cout_mutex);
        std::lock_guard<std::mutex> _2(cerr_mutex);

        log_to_stdouterr = (path == rocket::cst::std_out || path == rocket::cst::std_err);

        std_outstm.close();
        std_errstm.close();

        std_outstm.open(path);
        std_errstm.open(path);
    }

    void register_argument(std::string arg, std::function<void()> cb, std::string description) {
        custom_argument_t a;
        a.arg = arg;
        a.cb = [cb](std::optional<std::string>) {
            cb();
        };
        a.description = description;
        a.value = false;
        registered_arguments.push_back(a);
    }

    void register_argument(std::string arg, std::function<void(std::string value)> cb, std::string description, std::string value_type) {
        custom_argument_t a;
        a.arg = arg;
        a.cb = [cb](std::optional<std::string> v) {
            cb(*v);
        };
        a.description = description;
        a.value_type = value_type;
        a.value = true;
        registered_arguments.push_back(a);
    }

    void register_libattribution(std::string lib, std::string license) {
        registered_libattributions.push_back({ lib, license });
    }
}

void __rocket_premain(int argc, char **argv) {
    rocket::globals::g_main_thread_id = std::this_thread::get_id();
    rocket::globals::g_main_thread_id_set = true;

    rocket::globals::g_rocket_entrypoint_used = true;

    (void) argc; (void) argv;
}
