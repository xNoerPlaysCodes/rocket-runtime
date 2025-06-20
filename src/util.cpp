#include "util.hpp"
#include <algorithm>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/trigonometric.hpp>

std::vector<std::function<void()>> on_close_listeners = {};

std::vector<std::function<void(rocket::io::key_event_t)>> _key_listeners = {};
std::vector<std::function<void(rocket::io::mouse_event_t)>> _mouse_listeners = {};
std::vector<std::function<void(rocket::io::mouse_move_event_t)>> _mouse_move_listeners = {};

namespace util {

    bool is_glinit = false;

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

    std::string format_error(std::string error, int error_id, std::string error_source, std::string level) {
        std::stringstream sstream;
        sstream << "---\n";
        sstream << "[" << level << "]" << " " << "(" << error_source << ") " << "(" << "ID = " << error_id << ")";
        sstream << "\n";
        sstream << error;
        sstream << "\n";
        sstream << "---\n";
        return sstream.str();
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
}
