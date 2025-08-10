#include "util.hpp"
#include <algorithm>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/trigonometric.hpp>
#include <sstream>
#include <stack>

rocket::log_error_callback_t lerror_cb;
rocket::log_callback_t log_cb;

std::vector<std::function<void()>> on_close_listeners = {};

std::vector<std::function<void(rocket::io::key_event_t)>> _key_listeners = {};
std::vector<std::function<void(rocket::io::mouse_event_t)>> _mouse_listeners = {};
std::vector<std::function<void(rocket::io::mouse_move_event_t)>> _mouse_move_listeners = {};

std::unordered_map<rocket::io::keyboard_key, rocket::io::keystate_t> kstates;
std::unordered_map<rocket::io::mouse_button, rocket::io::keystate_t> mstates;

std::stack<char> chars_typed;

namespace util {

    bool is_glinit = false;
    rocket::log_level_t log_level = rocket::log_level_t::fatal;

    bool is_wayland() {
#ifdef __linux__
        if (const char *session = std::getenv("XDG_SESSION_TYPE")) {
            return std::string(session) == "wayland";
        }
        return false;
#else
        return false;
#endif
    }

    std::string fmtd_time_str() {
        std::time_t now = std::time(nullptr);
        std::tm local_tm;

        #ifdef _WIN32
            localtime_s(&local_tm, &now);   // Windows secure version
        #else
            localtime_r(&now, &local_tm);   // POSIX thread-safe version
        #endif

        std::ostringstream oss;
        oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    std::string format_error(std::string error, int error_id, std::string error_source, std::string level) {
        if (lerror_cb != nullptr) {
            lerror_cb(error, error_id, error_source, level);
            return "";
        }
        rocket::log_level_t elevel = rocket::log_level_t::fatal;
        if (level == "warn") {
            elevel = rocket::log_level_t::warn;
        } else if (level == "fatal") {
            elevel = rocket::log_level_t::fatal;
        } else if (level == "fatal_to_function" || level == "fatal-to-function") {
            elevel = rocket::log_level_t::fatal_to_function;
        }
        if (elevel > log_level) {
            return "";
        }
        std::stringstream sstream;
        sstream << "---\n";
        sstream << '[' << fmtd_time_str() << ']' << ' ' << "[" << level << "]" << " " << "(" << error_source << ") " << "(" << "ID = " << error_id << ")";
        sstream << "\n";
        sstream << error;
        sstream << "\n";
        sstream << "---\n";
        return sstream.str();
    }

    std::string format_log(std::string log, std::string class_file_library_source, std::string function_source, std::string level) {
        if (log_cb != nullptr) {
            log_cb(log, class_file_library_source, function_source, level);
            return "";
        }
        rocket::log_level_t elvl;
        if (level == "info") {
            elvl = rocket::log_level_t::info;
        } else if (level == "debug") {
            elvl = rocket::log_level_t::debug;
        } else if (level == "trace") {
            elvl = rocket::log_level_t::trace;
        }
        if (elvl > log_level) {
            return "";
        }
        std::stringstream ss;
        ss << '[' << fmtd_time_str() << ']' << ' '
            << '(' << class_file_library_source << "::" << function_source << ')' << ' '
            << "->" << ' '
            << log
            << '\n';
        return ss.str();
    }

    void set_log_callback(rocket::log_callback_t callback) {
        log_cb = callback;
    }

    void set_log_error_callback(rocket::log_error_callback_t callback) {
        lerror_cb = callback;
    }

    void gl_setup_perspective(rocket::vec3f_t size, float fov) {
        float aspect = size.x / size.y;
        fov = glm::radians(std::clamp(fov, 1.f, 179.f));
        float near_plane = 0.1f;
        float far_plane = 100.0f;

        glm::mat4 projection = glm::perspective(fov, aspect, near_plane, far_plane);
        // GLint u_projection = glGetUniformLocation(shader_program, "u_projection");
        // glUniformMatrix4fv(u_projection, 1, GL_FALSE, glm::value_ptr(projection));
        // ^^ Shader ^^
    }

    void gl_setup_ortho(rocket::vec2i_t size) {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        glOrtho(0, size.x, size.y, 0, -1, 1);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }

    void close_callback(GLFWwindow *) {
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

    void dispatch_event(rocket::io::key_event_t event) {
        for (auto l : _key_listeners) {
            l(event);
        }
    }

    void dispatch_event(rocket::io::mouse_event_t event) {
        for (auto l : _mouse_listeners) {
            l(event);
        }
    }

    void dispatch_event(rocket::io::mouse_move_event_t event) {
        for (auto l : _mouse_move_listeners) {
            l(event);
        }
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
}
