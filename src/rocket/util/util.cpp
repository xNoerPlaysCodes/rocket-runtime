#include "util.hpp"
#include "rocket/io.hpp"
#include "rocket/runtime.hpp"
#include <algorithm>
#include <condition_variable>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/trigonometric.hpp>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <stack>
#include <thread>

rocket::log_callback_t log_cb;

std::vector<std::function<void()>> on_close_listeners = {};

std::vector<std::function<void(rocket::io::key_event_t)>> _key_listeners = {};
std::vector<std::function<void(rocket::io::mouse_event_t)>> _mouse_listeners = {};
std::vector<std::function<void(rocket::io::mouse_move_event_t)>> _mouse_move_listeners = {};
std::vector<std::function<void(rocket::io::scroll_offset_event_t)>> _scroll_offset_listeners = {};

std::unordered_map<rocket::io::keyboard_key, rocket::io::keystate_t> kstates;
std::unordered_map<rocket::io::mouse_button, rocket::io::keystate_t> mstates;

std::vector<rocket::io::key_event_t> simulated_kevents;
std::vector<rocket::io::mouse_event_t> simulated_mevents;
std::vector<rocket::io::mouse_move_event_t> simulated_mmevents;
std::vector<rocket::io::scroll_offset_event_t> simulated_sevents;

std::stack<char> chars_typed;

util::membuf_t membuf;

namespace util {
    bool is_glinit = false;
#ifdef ROCKETGE__DEBUG_BUILD
    rocket::log_level_t log_level = rocket::log_level_t::all;
#else
    rocket::log_level_t log_level = rocket::log_level_t::info;
#endif

    bool is_wayland() {
#ifdef ROCKETGE__Platform_Linux
        if (const char *session = std::getenv("XDG_SESSION_TYPE")) {
            return std::string(session) == "wayland";
        }
        return false;
#else
        return false;
#endif
    }

    bool is_x11() {
#ifdef __linux__
        if (const char *session = std::getenv("XDG_SESSION_TYPE")) {
            return std::string(session) == "x11";
        }
        return false;
#else
        return false;
#endif
    }

    std::string fmtd_time_str() {
        std::time_t now = std::time(nullptr);
        std::tm local_tm;

        #ifdef ROCKETGE__Platform_Windows
            localtime_s(&local_tm, &now);   // Windows secure version
        #else
            localtime_r(&now, &local_tm);   // POSIX thread-safe version
        #endif

        std::ostringstream oss;
        oss << std::put_time(&local_tm, "%H:%M:%S");
        return oss.str();
    }

    std::string format_log(std::string log, std::string class_file_library_source, std::string function_source, std::string level) {
        if (log_cb != nullptr) {
            log_cb(log, class_file_library_source, function_source, level);
            return "";
        }
        rocket::log_level_t elvl = rocket::log_level_t::info;
        if (level == "info") {
            elvl = rocket::log_level_t::info;
        } else if (level == "debug") {
            elvl = rocket::log_level_t::debug;
        } else if (level == "trace") {
            elvl = rocket::log_level_t::trace;
        } else if (level == "all") {
            elvl = rocket::log_level_t::all;
        } else if (level == "warn" || level == "warning") {
            elvl = rocket::log_level_t::warn;
        } else if (level == "fatal") {
            elvl = rocket::log_level_t::fatal;
        } else if (level == "fatal_to_function" || level == "error" || level == "error") {
            elvl = rocket::log_level_t::error;
        } else if (level == "all") {
            elvl = rocket::log_level_t::all;
        } else if (level == "fixme") {
            elvl = rocket::log_level_t::warn;
        }
        if (!get_clistate().logall) {
            if (elvl < log_level) {
                return "";
            }
        }
        std::stringstream ss;
        ss << '[' << fmtd_time_str() << ']' << ' '
            << "[" << log_level_to_str(elvl) << "]" << ' '
            << '(' << class_file_library_source << "::" << function_source << ')' << ' '
            << log
            << '\n';
        return ss.str();
    }

    void set_log_callback(rocket::log_callback_t callback) {
        log_cb = callback;
    }

    void close_callback() {
        for (auto l : on_close_listeners) {
            l();
        }
    }

    std::vector<std::function<void()>> &get_on_close_listeners() {
        return on_close_listeners;
    }

    rocket::vec4<float> glify_a(rocket::rgba_color color) {
        return { color.x / 255.0f, color.y / 255.0f, color.z / 255.0f, color.w / 255.0f };
    }

    rocket::vec3<float> glify(rocket::rgb_color color) {
        return { color.x / 255.0f, color.y / 255.0f, color.z / 255.0f };
    }

    std::vector<std::function<void(rocket::io::key_event_t)>> &key_listeners() {
        return _key_listeners;
    }

    std::vector<std::function<void(rocket::io::mouse_event_t)>> &mouse_listeners() {
        return _mouse_listeners;
    }

    std::vector<std::function<void(rocket::io::mouse_move_event_t)>> &mouse_move_listeners() {
        return _mouse_move_listeners;
    }

    std::vector<std::function<void(rocket::io::scroll_offset_event_t)>> &scroll_offset_listeners() {
        return _scroll_offset_listeners;
    }

    bool key_down(rocket::io::keyboard_key key) {
        return kstates[key].down();
    }

    bool key_pressed(rocket::io::keyboard_key key) {
        return kstates[key].pressed();
    }

    bool key_released(rocket::io::keyboard_key key) {
        return kstates[key].released();
    }

    bool mouse_down(rocket::io::mouse_button button) {
        return mstates[button].down();
    }

    bool mouse_pressed(rocket::io::mouse_button button) {
        return mstates[button].pressed();
    }

    bool mouse_released(rocket::io::mouse_button button) {
        return mstates[button].released();
    }

    rocket::vec2<double> mouse_pos() {
        double x, y;
        glfwGetCursorPos(glfwGetCurrentContext(), &x, &y);
        return { x, y };
    }

    void dispatch_event(rocket::io::key_event_t event, bool addsm) {
        for (auto l : _key_listeners) {
            l(event);
            if (addsm) {
                simulated_kevents.push_back(event);
            }
        }
    }

    void dispatch_event(rocket::io::mouse_event_t event, bool addsm) {
        for (auto l : _mouse_listeners) {
            l(event);
            if (addsm) {
                simulated_mevents.push_back(event);
            }
        }
    }

    void dispatch_event(rocket::io::mouse_move_event_t event, bool addsm) {
        for (auto l : _mouse_move_listeners) {
            l(event);
            if (addsm) {
                simulated_mmevents.push_back(event);
            }
        }
    }

    void dispatch_event(rocket::io::scroll_offset_event_t event, bool addsm) {
        for (auto l : _scroll_offset_listeners) {
            l(event);
            if (addsm) {
                simulated_sevents.push_back(event);
            }
        }
    }

    std::vector<rocket::io::key_event_t> get_simulated_kevents() {
        return simulated_kevents;
    }

    std::vector<rocket::io::mouse_event_t> get_simulated_mevents() {
        return simulated_mevents;
    }

    std::vector<rocket::io::mouse_move_event_t> get_simulated_mmevents() {
        return simulated_mmevents;
    }

    std::vector<rocket::io::scroll_offset_event_t> get_simulated_sevents() {
        return simulated_sevents;
    }

    bool glinitialized() {
        return is_glinit;
    }

    void glinit(bool x) {
        is_glinit = x;
    }

    void set_log_level(rocket::log_level_t level) {
        log_level = level;
    }

    void io_update_end_frame() {
        for (auto &k : kstates) {
            k.second.previous = k.second.current;
            k.second.current = glfwGetKey(glfwGetCurrentContext(), static_cast<int>(k.first)) == GLFW_PRESS;
        }
        for (auto &m : mstates) {
            m.second.previous = m.second.current;
            m.second.current = glfwGetMouseButton(glfwGetCurrentContext(), static_cast<int>(m.first)) == GLFW_PRESS;
        }

        simulated_kevents.clear();
        simulated_mevents.clear();
        simulated_mmevents.clear();
        simulated_sevents.clear();
    }

    void push_formatted_char_typed(char c) {
        chars_typed.push(c);
    }

    char get_formatted_char_typed() {
        if (chars_typed.size() == 0) {
            return '\0';
        }

        char c = '\0';

        c = chars_typed.top();
        chars_typed.pop();

        return c;
    }

    rocket::renderer_2d *global_renderer_2d = nullptr;

    void set_global_renderer_2d(rocket::renderer_2d *renderer) {
        global_renderer_2d = renderer;
    }

    rocket::renderer_2d *get_global_renderer_2d() {
        return global_renderer_2d;
    }

    global_state_cliargs_t clistate = {};
    void init_clistate(global_state_cliargs_t args) {
        ::util::clistate = args;
    }

    global_state_cliargs_t get_clistate() {
        return ::util::clistate;
    }

    std::vector<std::string> split(const std::string& s, char delim) {
        std::vector<std::string> out;
        std::stringstream ss(s);
        std::string item;

        while (std::getline(ss, item, delim))
            out.push_back(item);

        return out;
    }

    bool validate_gl_version_string(std::string string) {
        float val = std::stof(string);
        return 
            (val == 1.0f || val == 1.1f || val == 1.2f || val == 1.3f || val == 1.4f || val == 1.5f) ||
            (val == 2.0f || val == 2.1f) ||
            (val == 3.0f || val == 3.1f || val == 3.2f || val == 3.3f) ||
            (val == 4.0f || val == 4.1f || val == 4.2f || val == 4.3f || val == 4.4f || val == 4.5f || val == 4.6f);
    }

    void init_memory_buffer() {
        constexpr size_t sz = 16 * 1024 * 1024;
        membuf.mem = new uint8_t[sz];
        membuf.sz = sz;
    }

    membuf_t *get_memory_buffer() {
        return &membuf;
    }
}
