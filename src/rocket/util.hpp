#ifndef ROCKETGE__IMPL__UTIL_HPP
#define ROCKETGE__IMPL__UTIL_HPP

#include "../../include/rocket/types.hpp"
#include "../../include/rocket/io.hpp"
#include "../../include/rocket/runtime.hpp"
#include "rocket/renderer.hpp"
#include <GLFW/glfw3.h>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

#define GL_VERSION_UNK 0
#define GL_VERSION_20 20
#define GL_VERSION_21 21
#define GL_VERSION_30 30
#define GL_VERSION_31 31
#define GL_VERSION_32 32
#define GL_VERSION_33 33
#define GL_VERSION_40 40
#define GL_VERSION_41 41
#define GL_VERSION_42 42
#define GL_VERSION_43 43
#define GL_VERSION_44 44
#define GL_VERSION_45 45
#define GL_VERSION_46 46

namespace util {
    struct global_state_cliargs_t {
        bool noplugins = false;
        bool logall = false;
        bool debugoverlay = false;
        bool nosplash = false;
        bool dx11 = false;
        bool dx12 = false;
        bool opengl = true;
        int glversion = GL_VERSION_UNK;
    };

    std::string format_error(std::string error, int error_id, std::string error_source, std::string level);
    std::string format_log(std::string log, std::string class_file_library_source, std::string function_source, std::string level);

    void set_log_error_callback(rocket::log_error_callback_t);
    void set_log_callback(rocket::log_callback_t);

    void close_callback();
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

    void set_global_renderer_2d(rocket::renderer_2d *renderer);
    rocket::renderer_2d *get_global_renderer_2d();

    void init_clistate(global_state_cliargs_t);
    global_state_cliargs_t get_clistate();
}

#endif
