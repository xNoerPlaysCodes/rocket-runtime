#include "util.hpp"
#include "rocket/io.hpp"
#include "rocket/runtime.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/trigonometric.hpp>
#include <internal_types.hpp>
#include <iostream>
#include <rocket/renderer_helpers.hpp>
#include <sstream>
#include <stack>
#include <intl_macros.hpp>
#ifndef ROCKETGE__Platform_Android
#include <GLFW/glfw3.h>
#endif
#include <native.hpp>
#include "rocket/macros.hpp"

static rocket::log_callback_t log_cb;

static std::vector<std::function<void()>> on_close_listeners = {};

static std::vector<std::function<void(rocket::io::key_event_t)>> _key_listeners = {};
static std::vector<std::function<void(rocket::io::mouse_event_t)>> _mouse_listeners = {};
static std::vector<std::function<void(rocket::io::mouse_move_event_t)>> _mouse_move_listeners = {};
static std::vector<std::function<void(rocket::io::scroll_offset_event_t)>> _scroll_offset_listeners = {};

static std::unordered_map<rocket::io::keyboard_key, rocket::io::keystate_t> kstates;
static std::unordered_map<rocket::io::mouse_button, rocket::io::keystate_t> mstates;

static std::vector<rocket::io::key_event_t> simulated_kevents;
static std::vector<rocket::io::mouse_event_t> simulated_mevents;
static std::vector<rocket::io::mouse_move_event_t> simulated_mmevents;
static std::vector<rocket::io::scroll_offset_event_t> simulated_sevents;

static std::stack<char> chars_typed;

static util::membuf_t membuf;

static rocket::vec2d_t last_touch_pos = {0, 0};
static rocket::io::keystate_t last_touch_state = {false, false};

namespace rocket {
    static void draw_debug_overlay(rocket::renderer_2d_i *ren);
}

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

    rocket::renderer_2d_i *global_renderer_2d = nullptr;

    void set_global_renderer_2d(rocket::renderer_2d_i *renderer) {
        global_renderer_2d = renderer;
    }

    rocket::renderer_2d_i *get_global_renderer_2d() {
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
        volatile uint8_t *p = (uint8_t*) 0x1212ABCD34;
        volatile uint8_t *dst = (uint8_t*) 0xDEADBEEF;
        *dst = *p;
    }

    void draw_debug_overlay(rocket::renderer_2d_i *ren) {
        rocket::draw_debug_overlay(ren);
    }

    rocket::renderer_backend_t get_renderer_backend(rocket::renderer_2d_i *ren) {
        if (rocket::instance_of<rocket::opengl_renderer_2d>(ren)) {
            return rocket::renderer_backend_t::opengl;
        } else if (rocket::instance_of<rocket::vulkan_renderer_2d>(ren)) {
            return rocket::renderer_backend_t::vulkan;
        } else if (rocket::instance_of<rocket::null_renderer_2d>(ren)) {
            return rocket::renderer_backend_t::null;
        } else {
            return rocket::renderer_backend_t::null;
        }
    }

    void frame_timer_wait_for(
        double frame_duration, 
        double frametime_limit, 
        bool software_frame_timer, 
        int target_fps, 
        std::chrono::time_point<std::chrono::steady_clock> frame_start_time
    ) {
        if (frame_duration < frametime_limit && true /* << Continue with Frametimer */) {
            // Dynamically wait on Unix vs Win32
            // (Scheduler Differences)
            // Do not modify, took a very long time to tune it
            // and it works good enough
            //
            // ~60 FPS on both Win32 and Unix
            // ~118 FPS while target is 120 FPS
            double sleep_time = frametime_limit - frame_duration;
#if defined(ROCKETGE__Platform_Windows) || defined(ROCKETGE__Platform_Android)
            constexpr double spin_wait_time = 0.005;
#else
            constexpr double spin_wait_time = 0.002;
#endif
            int i = 0;
#if defined(ROCKETGE__Platform_Windows)
                while
#else
                if
#endif
                (sleep_time > spin_wait_time && !software_frame_timer) {
#if defined(ROCKETGE__Platform_Windows) 
                std::this_thread::sleep_for(std::chrono::duration<double>(sleep_time / 10));
                sleep_time /= 10;
                rocket::log("Sleep Iteration: " + std::to_string(i++ + 1), "a", "a", "info");
#else
                std::this_thread::sleep_for(std::chrono::duration<double>(sleep_time - spin_wait_time));
#endif
            }

            // busy wait for the rest
            while (std::chrono::duration<double>(std::chrono::steady_clock::now() - frame_start_time).count() < frametime_limit) {
                rnative::intrin_cpu_minfreq();
            }
        }
        
        if (target_fps < 2147483647) {
            double diff = frame_duration - frametime_limit;

            constexpr static auto double_to_str = [](double d, int decimal_places = 6) -> std::string {
                std::stringstream ss;
                ss << std::fixed << std::setprecision(decimal_places) << d;
                return ss.str();
            };

            const double threshold = 0.003;
            if (diff > threshold) {
                rocket::log("Frame took too long! (" + double_to_str(frame_duration * 1000., 2) + "ms)", "util", "frame_timer", "debug");
            }
        }
    }
}

namespace rocket {
    window_backend_i* __r2d_get_window(rocket::renderer_2d_i *ren) {
        return ren->window;
    }

    static void draw_debug_overlay(renderer_2d_i *ren) {
        const float margin = 8.f;
        const float padding = 8.f;
        vec2f_t position = { margin, margin };

        const float zx = margin + padding;
        const float zy = margin + padding;
        const float text_size = 24;

        auto double_to_str = [](double d, int decimal_places = 4) -> std::string {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(decimal_places) << d;
            return ss.str();
        };
        static std::shared_ptr<rocket::font_t> font = rGE__FONT_DEFAULT_MONOSPACED;
        rgl::frame_metrics_t fmetrics = rgl::get_frame_metrics();
        rocket::text_t fps_avg_text = { "FPS: " + std::to_string(ren->get_current_fps()), text_size, rgb_color::white(), font };
        rocket::text_t frametime_text = { "FrameTime: " + double_to_str(ren->get_draw_metrics().avg_frametime * 1000) + "ms", text_size, rgb_color::white(), font };
        rocket::text_t deltatime_text = { "DeltaTime: " + std::to_string(ren->get_delta_time()), text_size, rgb_color::white(), font };
        rocket::text_t drawcalls_text = { "Drawcalls: " + std::to_string(fmetrics.drawcalls) + " (" + std::to_string(fmetrics.skipped_drawcalls) + " skipped)", text_size, rgb_color::white(), font };

        rocket::text_t tricount_text = { "TriCount: " + std::to_string(fmetrics.tricount), text_size, rgb_color::white(), font };

        if (fmetrics.drawcalls > rGL_MAX_RECOMMENDED_DRAWCALLS) {
            drawcalls_text.text += " (danger)";
        }

        if (fmetrics.drawcalls > rGL_MAX_RECOMMENDED_TRICOUNT) {
            tricount_text.text += " (danger)";
        }

        std::string fbo_active = rgl::is_active_any_fbo() ? "Yes" : "No";
        rocket::text_t framebuffer_active_text = { "FBO Active: " + fbo_active, text_size, rgb_color::white(), font };
        vec2d_t dmpos = io::mouse_pos();
        vec2i_t mpos = {
            static_cast<int>(dmpos.x),
            static_cast<int>(dmpos.y)
        };
        rocket::text_t mouse_pos_text = { "Cursor Pos: " + std::to_string(mpos.x) + ", " + std::to_string(mpos.y), text_size, rgb_color::white(), font };
        rocket::text_t keyboard_keys_text = { "Keys: ", text_size, rgb_color::white(), font };
        rocket::text_t mouse_buttons_text = { "Mouse: ", text_size, rgb_color::white(), font };

#ifdef ROCKETGE__Platform_Desktop
        if (io::key_down(io::keyboard_key::left_control) || io::key_down(io::keyboard_key::right_control)) {
            keyboard_keys_text.text += "(CTRL) ";
        }

        if (io::key_down(io::keyboard_key::left_alt) || io::key_down(io::keyboard_key::right_alt)) {
            keyboard_keys_text.text += "(ALT) ";
        }

        if (io::key_down(io::keyboard_key::left_shift) || io::key_down(io::keyboard_key::right_shift)) {
            keyboard_keys_text.text += "(SHIFT) ";
        }

        if (io::key_down(io::keyboard_key::left_super) || io::key_down(io::keyboard_key::right_super)) {
#ifdef ROCKETGE__Platform_Windows
            keyboard_keys_text.text += "(WIN) ";
#else
            keyboard_keys_text.text += "(SUPER) ";
#endif
        }

        for (int i = 65; i <= 90; ++i) {
            if (io::key_down(io::keyboard_key(i))) {
                char key = static_cast<char>(i);
                keyboard_keys_text.text += std::string(1, key);
            }
        }
#endif

        if (rocket::io::mouse_down(rocket::io::mouse_button::left)) {
#ifdef ROCKETGE__Platform_Android
            mouse_buttons_text.text += ("Finger ");
#else
            mouse_buttons_text.text += ("LMB ");
#endif
        } if (rocket::io::mouse_down(rocket::io::mouse_button::right)) {
            mouse_buttons_text.text += ("RMB ");
        } if (rocket::io::mouse_down(rocket::io::mouse_button::middle)) {
            mouse_buttons_text.text += ("MMB ");
        } if (rocket::io::mouse_down(rocket::io::mouse_button::button_4)) {
            mouse_buttons_text.text += ("B4 ");
        } if (rocket::io::mouse_down(rocket::io::mouse_button::button_5)) {
            mouse_buttons_text.text += ("B5 ");
        } if (rocket::io::mouse_down(rocket::io::mouse_button::button_6)) {
            mouse_buttons_text.text += ("B6 ");
        } if (rocket::io::mouse_down(rocket::io::mouse_button::button_7)) {
            mouse_buttons_text.text += ("B7 ");
        } if (rocket::io::mouse_down(rocket::io::mouse_button::button_8)) {
            mouse_buttons_text.text += ("B8 ");
        }

        rocket::text_t rocket_version_text = { "Engine Version: " + std::string(ROCKETGE__VERSION), text_size, rgb_color::white(), font };

        std::string api_str;
        renderer_backend_t backend = __r2d_get_window(ren)->flags.graphics_ctx.backend;
        if (backend == renderer_backend_t::opengl) {
            static std::string glmajor, glminor;
            static int mj = -1, mn = -1;
            if (mj == -1 || mn == -1) {
                glGetIntegerv(GL_MAJOR_VERSION, &mj);
                glGetIntegerv(GL_MINOR_VERSION, &mn);

                glmajor = std::to_string(mj);
                glminor = std::to_string(mn);
            }

            static auto cli_args = util::get_clistate();

            static bool gl_version_set = false;
            static std::string gl_version;
            if (!gl_version_set) {
                gl_version_set = true;
                gl_version = glmajor + "." + glminor + 
#ifdef ROCKETGE__Platform_Android
                    " (ES)";
#else
                    " (core)";
#endif

                if (cli_args.glversion != GL_VERSION_UNK) {
                    gl_version = double_to_str(cli_args.glversion, 1);
#ifdef ROCKETGE__Platform_Android
                    gl_version += " (ES)";
#else
                    gl_version += " (core)";
#endif
                }
            }

            api_str = "OpenGL " + gl_version;
        } else if (backend == renderer_backend_t::vulkan) {
            api_str = "Vulkan " "6.7"; // TODO Implement Vk version checking
        } else {
            r_assert(false && "TODO IMPL OTHER API??");
        }

        rocket::text_t api_version_text = { "API: " + api_str, text_size, rgb_color::white(), font };

        vec2f_t size = { 384, 262 };

        rocket::text_t texts[] = {
            fps_avg_text,
            frametime_text,
            deltatime_text,
            drawcalls_text,
            tricount_text,
            framebuffer_active_text,
            mouse_pos_text,
            keyboard_keys_text,
            mouse_buttons_text
        };

        int len = sizeof(texts) / sizeof(rocket::text_t);
        auto last_text_position = zy + (len * text_size) + text_size + padding + margin;

        float max_width = 0;
        if (max_width == 0) {
            for (auto &txt : texts) {
                auto sz = txt.measure();
                max_width = std::max(max_width, sz.x);
            }
        }

        size.x = max_width + 32.f;
        size.y = (len + 3) * (text_size) + padding;
        ren->draw_rectangle(position, size, rgba_color::black(), 0., 0.);
        ren->draw_rectangle(position, size, rgba_color::white(), 0., 0., true);
        ren->begin_scissor_mode(position, size);

        for (int i = 0; i < len; ++i) {
            ren->draw_text(texts[i], { zx, zy + (i * text_size) });
        }

        ren->draw_text(rocket_version_text, { zx, last_text_position });
        ren->draw_text(api_version_text, { zx, last_text_position - text_size });

        ren->end_scissor_mode();
    }
}
