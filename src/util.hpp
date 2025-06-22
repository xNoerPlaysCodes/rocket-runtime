#ifndef ROCKETGE__IMPL__UTIL_HPP
#define ROCKETGE__IMPL__UTIL_HPP

#include "../include/rocket/types.hpp"
#include "../include/rocket/io.hpp"
#include "../include/rocket/runtime.hpp"
#include <GLFW/glfw3.h>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace util {
    std::string format_error(std::string error, int error_id, std::string error_source, std::string level);

    void gl_setup_ortho(rocket::vec2i_t size);
    void gl_setup_perspective(rocket::vec3f_t size, float fov);

    void close_callback(GLFWwindow *);
    std::vector<std::function<void()>> &get_on_close_listeners();

    rocket::vec4<float> glify_a(rocket::rgba_color);
    rocket::vec3<float> glify(rocket::rgb_color);

    std::vector<std::function<void(rocket::io::key_event_t)>> &key_listeners();
    std::vector<std::function<void(rocket::io::mouse_event_t)>> &mouse_listeners();
    std::vector<std::function<void(rocket::io::mouse_move_event_t)>> &mouse_move_listeners();

    void dispatch_event(rocket::io::key_event_t);
    void dispatch_event(rocket::io::mouse_event_t);
    void dispatch_event(rocket::io::mouse_move_event_t);

    bool glinitialized();
    void glinit(bool);

    bool is_wayland();

    void set_log_level(rocket::log_level_t level);
}

#endif
