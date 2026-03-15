#include "util.hpp"
#include "rocket/io.hpp"
#include "rocket/runtime.hpp"
#include <algorithm>
#include <condition_variable>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/trigonometric.hpp>
#include <internal_types.hpp>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <stack>
#include <thread>
#ifndef ROCKETGE__Platform_Android
#include <GLFW/glfw3.h>
#endif
#include "intl_macros.hpp"
#include <native.hpp>
#include "rocket/macros.hpp"

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

void* (*new_op)(std::size_t) = ::operator new;
void* (*newarr_op)(std::size_t) = ::operator new[];
void (*del_op)(void*) = ::operator delete;
void (*delarr_op)(void*) = ::operator delete[];

static rocket::vec2d_t last_touch_pos = {0, 0};
static rocket::io::keystate_t last_touch_state = {false, false};

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

        native_localtime(&now, &local_tm);

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
            if (l == nullptr) continue;
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
#ifndef ROCKETGE__Platform_Android
        double x, y;
        if (glfwGetCurrentContext() == nullptr) {
            return { 0, 0 };
        } else {
            glfwGetCursorPos(glfwGetCurrentContext(), &x, &y);
        }
        return { x, y };
#endif
        // bool sdl2_get_touch_position = false;
        // r_assert(sdl2_get_touch_position);
        return {0, 0};
    }

    void dispatch_event(rocket::io::key_event_t event, bool addsm) {
        for (auto l : _key_listeners) {
            l(event);
        }


        if (addsm) {
            simulated_kevents.push_back(event);
        }
    }

    void dispatch_event(rocket::io::mouse_event_t event, bool addsm) {
        for (auto l : _mouse_listeners) {
            l(event);
        }
        if (addsm) {
            simulated_mevents.push_back(event);
        }
    }

    void dispatch_event(rocket::io::mouse_move_event_t event, bool addsm) {
        for (auto l : _mouse_move_listeners) {
            l(event);
        }

        if (addsm) {
            simulated_mmevents.push_back(event);
        }
    }

    void dispatch_event(rocket::io::scroll_offset_event_t event, bool addsm) {
        for (auto l : _scroll_offset_listeners) {
            l(event);
        }

        if (addsm) {
            simulated_sevents.push_back(event);
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

    rocket::vec2d_t get_last_touch_pos() { return last_touch_pos; }
    void set_last_touch_pos(rocket::vec2d_t pos) { last_touch_pos = pos; }

    rocket::io::keystate_t get_last_touch_state() { return last_touch_state; }
    void set_last_touch_state(rocket::io::keystate_t state) { last_touch_state = state; }

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
#ifndef ROCKETGE__Platform_Android
        if (glfwGetCurrentContext() == nullptr) goto cleanup;
        for (auto &k : kstates) {
            k.second.previous = k.second.current;
            k.second.current = glfwGetKey(glfwGetCurrentContext(), static_cast<int>(k.first)) == GLFW_PRESS;
        }
        for (auto &m : mstates) {
            m.second.previous = m.second.current;
            m.second.current = glfwGetMouseButton(glfwGetCurrentContext(), static_cast<int>(m.first)) == GLFW_PRESS;
        }
#endif

        goto cleanup;

cleanup:
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

    bool validate_gl_version_string(std::string s) {
        int major, minor;
        if (sscanf(s.c_str(), "%d.%d", &major, &minor) != 2)
            return false;
        switch (major) {
            case 1: return minor >= 0 && minor <= 5;
            case 2: return minor == 0 || minor == 1;
            case 3: return minor >= 0 && minor <= 3;
            case 4: return minor >= 0 && minor <= 6;
            default: return false;
        }
    }

    void init_memory_buffer() {
        constexpr size_t sz = 4 * 1024 * 1024;
        membuf.mem = new uint8_t[sz];
        membuf.sz = sz;
    }

    membuf_t *get_memory_buffer() {
        return &membuf;
    }

    timer_t::timer_t(bool start) {
        if (start) this->start();
    }

    void timer_t::start() {
        start_time = clock::now();
    }

    void timer_t::stop() {
        end_time = clock::now();
        this->ended = true;
    }

    timer_t::clock::duration timer_t::elapsed() {
        if (!this->ended) return clock::now() - this->start_time;
        return this->end_time - this->start_time;
    }

    double timer_t::ms() {
        return std::chrono::duration<double, std::milli>(elapsed()).count();
    }

    double timer_t::us() {
        return std::chrono::duration<double, std::micro>(elapsed()).count();
    }

    double timer_t::sec() {
        return std::chrono::duration<double>(elapsed()).count();
    }

    double timer_t::min() {
        return std::chrono::duration<double, std::ratio<60>>(elapsed()).count();
    }

    double timer_t::hr() {
        return std::chrono::duration<double, std::ratio<3600>>(elapsed()).count();
    }

    std::string timer_t::to_str(unit_flag_t flag) {
        std::ostringstream oss;

        double total_us = this->us();

        int days = 0, hrs = 0, mins = 0, secs = 0, ms = 0, us = 0;

        constexpr auto has_flag = [](unit_flag_t flag, unit_flag_t other) -> bool {
            return static_cast<uint8_t>(flag) & static_cast<uint8_t>(other);
        };

        if (has_flag(flag, unit_flag_t::day)) {
            days = static_cast<int>(total_us / (1000.0 * 1000 * 60 * 60 * 24));
            total_us -= days * 1000.0 * 1000 * 60 * 60 * 24;
        }

        if (has_flag(flag, unit_flag_t::hr)) {
            hrs = static_cast<int>(total_us / (1000.0 * 1000 * 60 * 60));
            total_us -= hrs * 1000.0 * 1000 * 60 * 60;
        }

        if (has_flag(flag, unit_flag_t::min)) {
            mins = static_cast<int>(total_us / (1000.0 * 1000 * 60));
            total_us -= mins * 1000.0 * 1000 * 60;
        }

        if (has_flag(flag, unit_flag_t::sec)) {
            secs = static_cast<int>(total_us / (1000.0 * 1000));
            total_us -= secs * 1000.0 * 1000;
        }

        if (has_flag(flag, unit_flag_t::ms)) {
            ms = static_cast<int>(total_us / 1000.0);
            total_us -= ms * 1000;
        }

        if (has_flag(flag, unit_flag_t::us)) {
            us = static_cast<int>(total_us);
        }

        if (days) oss << days << "days, ";
        if (hrs)  oss << hrs << "hrs, ";
        if (mins) oss << mins << "mins, ";
        if (secs) oss << secs << "secs, ";
        if (ms)   oss << ms << "ms, ";
        if (us)   oss << us << "us, ";

        std::string result = oss.str();
        if (!result.empty()) result.erase(result.size() - 2); // remove trailing ", "
        return result.empty() ? "0us" : result;
    }

    void segfault() {
        volatile int *p = (int*) 0x1;
        std::cout << *p;
    }
}

// struct alignas(std::max_align_t) memory_header_t {
//     size_t size_bytes;
// };
//
// std::atomic<int> allocations = 0;
// std::atomic<int> allocated_bytes = 0;
//
// void *operator new(std::size_t sz) {
//     memory_header_t *header = (memory_header_t*) malloc(sizeof(memory_header_t) + sz);
//     header->size_bytes = sz;
//
//     allocations++;
//     allocated_bytes += sz;
//
//     return header + 1;
// }
//
// void *operator new[](std::size_t sz) {
//     memory_header_t *header = (memory_header_t*) malloc(sizeof(memory_header_t) + sz);
//     header->size_bytes = sz;
//
//     allocations++;
//     allocated_bytes += sz;
//
//     return header + 1;
// }
//
// void operator delete(void *mem) noexcept {
//     if (mem == nullptr) return;
//     memory_header_t *header = (memory_header_t*) (((uint8_t*)mem) - sizeof(memory_header_t));
//
//     allocated_bytes -= header->size_bytes;
//     allocations--;
//
//     free(header);
// }
//
// void operator delete[](void* mem) noexcept {
//     if (mem == nullptr) return;
//     memory_header_t *header = (memory_header_t*) (((uint8_t*)mem) - sizeof(memory_header_t));
//
//     allocated_bytes -= header->size_bytes;
//     allocations--;
//
//     free(header);
// }
//
// void operator delete(void* mem, std::size_t sz) noexcept {
//     if (!mem) return;
//     memory_header_t* header = (memory_header_t*)((uint8_t*)mem - sizeof(memory_header_t));
//     allocated_bytes -= header->size_bytes;
//     allocations--;
//     free(header);
// }
//
// void operator delete[](void* mem, std::size_t sz) noexcept {
//     if (!mem) return;
//     memory_header_t* header = (memory_header_t*)((uint8_t*)mem - sizeof(memory_header_t));
//     allocated_bytes -= header->size_bytes;
//     allocations--;
//     free(header);
// }
//
// void* operator new(std::size_t sz, const std::nothrow_t&) noexcept {
//     try { return ::operator new(sz); }
//     catch(...) { return nullptr; }
// }
//
// void operator delete(void* mem, const std::nothrow_t&) noexcept {
//     ::operator delete(mem);
// }
