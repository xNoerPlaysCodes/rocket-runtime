#ifndef ROCKETGE__INTL_STATE_MANAGER_HPP
#define ROCKETGE__INTL_STATE_MANAGER_HPP

#include <functional>
#include <vector>
#include <rocket/io.hpp>

#warning Implement this and actually use it damn

// Backwards compatibility with old pieces of code
namespace util {
    std::vector<std::function<void()>> &get_on_close_listeners();

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

    void set_gl_initialized(bool);
    bool get_gl_initialized();

    char pop_formatted_char_typed();
    void push_formatted_char_typed(char);
}

#endif//ROCKETGE__INTL_STATE_MANAGER_HPP
