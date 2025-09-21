#ifndef ROCKETGE__IMPL__UTIL_HPP
#define ROCKETGE__IMPL__UTIL_HPP

#include "../../include/rocket/types.hpp"
#include "../../include/rocket/io.hpp"
#include "../../include/rocket/runtime.hpp"
#include <GLFW/glfw3.h>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace util {
    std::string format_error(std::string error, int error_id, std::string error_source, std::string level);
    std::string format_log(std::string log, std::string class_file_library_source, std::string function_source, std::string level);

    void set_log_error_callback(rocket::log_error_callback_t);
    void set_log_callback(rocket::log_callback_t);

    void close_callback(GLFWwindow *);
    std::vector<std::function<void()>> &get_on_close_listeners();

    rocket::vec4<float> glify_a(rocket::rgba_color);
    rocket::vec3<float> glify(rocket::rgb_color);

    std::vector<std::function<void(rocket::io::key_event_t)>> &key_listeners();
    std::vector<std::function<void(rocket::io::mouse_event_t)>> &mouse_listeners();
    std::vector<std::function<void(rocket::io::mouse_move_event_t)>> &mouse_move_listeners();
    std::vector<std::function<void(rocket::io::scroll_offset_event_t)>> &scroll_offset_listeners();

    void dispatch_event(rocket::io::key_event_t, bool add_simulated = false);
    void dispatch_event(rocket::io::mouse_event_t, bool add_simulated = false);
    void dispatch_event(rocket::io::mouse_move_event_t, bool add_simulated = false);
    void dispatch_event(rocket::io::scroll_offset_event_t, bool add_simulated = false);

    std::vector<rocket::io::key_event_t> get_simulated_kevents();
    std::vector<rocket::io::mouse_event_t> get_simulated_mevents();
    std::vector<rocket::io::mouse_move_event_t> get_simulated_mmevents();
    std::vector<rocket::io::scroll_offset_event_t> get_simulated_sevents();

    bool glinitialized();
    void glinit(bool);

    bool is_wayland();
    bool is_x11();

    void set_log_level(rocket::log_level_t level);

    void io_update_end_frame();

    bool key_down(rocket::io::keyboard_key key);
    bool key_pressed(rocket::io::keyboard_key key);
    bool key_released(rocket::io::keyboard_key key);

    rocket::vec2<double> mouse_pos();

    bool mouse_down(rocket::io::mouse_button button);
    bool mouse_pressed(rocket::io::mouse_button button);
    bool mouse_released(rocket::io::mouse_button button);

    char get_formatted_char_typed();
    void push_formatted_char_typed(char c);
}

#endif
