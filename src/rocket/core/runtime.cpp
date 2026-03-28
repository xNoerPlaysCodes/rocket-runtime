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
#include "rust/runtime.h"

#ifdef ROCKETGE__Platform_Android
#include <android/log.h>
#endif
//di rust calls
extern "C" {
    void rs_rocket_log(const char *log, const char *source, const char *func, const char *level) {
        rocket::log(log, source, func, level);
    }
    void rs_rocket_exit(int status_code) {
        rocket::exit(status_code);
    }
}

namespace rocket {
    void set_cli_arguments(int argc, char *argv[]) {
        RocketCliResult r = parse_rocket_arguments(argc, (const char* const*)argv);
        if (r.should_exit) {
            rocket::exit(r.exit_code);
        }
        util::global_state_cliargs_t args = {};
        args.noplugins = r.noplugins;
        args.logall = r.logall;
        args.debugoverlay = r.debugoverlay;
        args.nosplash = r.nosplash;
        args.notext = r.notext;
        args.forcewayland = r.forcewayland;
        args.software_frame_timer = r.software_frame_timer;

        args.glversion = static_cast<float>(r.glversion);
        args.framerate = r.framerate;
        args.viewport_size_set = r.viewport_size_set;
        if (r.viewport_size_set) {
            args.viewport_size = std::string(r.viewport_size);
        }
        args.opengl = true;
        args.dx11 = false;
        args.dx12 = false;
        args.lognone = false;
        util::init_clistate(args);
    }
}
/////////////////////////////////
// i dont like this one bit   ///
// cpp rs contract            ///
/*extern "C{                  /////////////////////////////////////
    struct RocketCliResult {                                    ///
        bool noplugins;                                         ///
        bool logall;                                            ///
        bool debugoverlay;                                      ///
        bool nosplash;                                          ///
        bool notext;                                            ///
        bool forcewayland;                                      ///
        bool software_frame_timer;                              ///
        int32_t glversion;                                      ///
        int32_t framerate;                                      ///
                                                                ///
        // [}{}[][]]]{)_}                                       ///
        bool should_exit;                                       ///
        int32_t exit_code;                                      ///
    };                                                          ///
                                                                ///
    // uh this is similar to Lua++ expired extern feat-         ///
    RocketCliResult parse_cli_in_rust(int argc, char** argv);   ///
*/

                                                             ///
///////////////////////////////////////////////////////////////////

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
        } else {
            std::exit(status_code);
        }
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
