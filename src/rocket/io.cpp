#include "../../include/rocket/io.hpp"
#include "util.hpp"

namespace rocket {
    namespace io {
        bool keystate_t::pressed() const { return current && !previous; }
        bool keystate_t::released() const { return !current && previous; }
        bool keystate_t::down() const { return current; }

        void add_listener(std::function<void(key_event_t)> listener) {
            ::util::key_listeners().push_back(listener);
        }

        void add_listener(std::function<void(mouse_event_t)> listener) {
            ::util::mouse_listeners().push_back(listener);
        }

        void add_listener(std::function<void(mouse_move_event_t)> listener) {
            ::util::mouse_move_listeners().push_back(listener);
        }
    }
    // IMMD IO
    namespace io {
        bool key_pressed(keyboard_key key) {
            return ::util::key_pressed(key);
        }

        bool key_down(keyboard_key key) {
            return ::util::key_down(key);
        }

        bool key_released(keyboard_key key) {
            return ::util::key_released(key);
        }

        bool mouse_pressed(mouse_button button) {
            return ::util::mouse_pressed(button);
        }

        bool mouse_down(mouse_button button) {
            return ::util::mouse_down(button);
        }

        bool mouse_released(mouse_button button) {
            return ::util::mouse_released(button);
        }

        rocket::vec2d_t mouse_pos() {
            return ::util::mouse_pos();
        }

        rocket::vec2f_t mouse_pos_f() {
            return {static_cast<float>(::util::mouse_pos().x), static_cast<float>(::util::mouse_pos().y)};
        }

        char get_formatted_char_typed() {
            return ::util::get_formatted_char_typed();
        }
    }
}
