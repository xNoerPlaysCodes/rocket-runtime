#include "../../include/rocket/io.hpp"
#include "rocket/types.hpp"
#include "util.hpp"

#include <GLFW/glfw3.h>

namespace rocket {
    namespace io {
        bool keystate_t::pressed() const { return current && !previous; }
        bool keystate_t::released() const { return !current && previous; }
        bool keystate_t::down() const { return current; }

        keystate_t keystate_t::make_pressed() { return {true, false}; }
        keystate_t keystate_t::make_released() { return {false, true}; }
        keystate_t keystate_t::make_down() { return {true, true}; }

        void add_listener(std::function<void(key_event_t)> listener) {
            ::util::key_listeners().push_back(listener);
        }

        void add_listener(std::function<void(mouse_event_t)> listener) {
            ::util::mouse_listeners().push_back(listener);
        }

        void add_listener(std::function<void(mouse_move_event_t)> listener) {
            ::util::mouse_move_listeners().push_back(listener);
        }

        void add_listener(std::function<void(scroll_offset_event_t)> listener) {
            ::util::scroll_offset_listeners().push_back(listener);
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

        bool key_state(keyboard_key key, keystate_t state) {
            if (state == keystate_t::make_down()) {
                return ::util::key_down(key);
            } else if (state == keystate_t::make_pressed()) {
                return ::util::key_pressed(key);
            } else if (state == keystate_t::make_released()) {
                return ::util::key_released(key);
            }
            return false;
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

        bool mouse_state(mouse_button button, keystate_t state) {
            if (state == keystate_t::make_down()) {
                return ::util::mouse_down(button);
            } else if (state == keystate_t::make_pressed()) {
                return ::util::mouse_pressed(button);
            } else if (state == keystate_t::make_released()) {
                return ::util::mouse_released(button);
            }
            return false;
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

    namespace gpad {
        bool is_available(int id) {
            return glfwJoystickIsGamepad(id);
        }
        gamepad_t get_handle(int id) {
            if (!is_available(id)) {
                rocket::log_error("Gamepad not available with ID: " + std::to_string(id), -1, "rocket::gpad::get_handle", "warning");
                return 255;
            }

            return static_cast<gamepad_t>(id);
        }

        std::string get_human_readable_name(button_t button, style_t style) {
            return "Unknown";
        }
        std::string get_human_readable_name(axis_t, style_t) {
            return "Unknown";
        }

        float get_axis_state(gamepad_t g, axis_t a, float deadzone) {
            GLFWgamepadstate state;
            glfwGetGamepadState(g, &state);
            int glfw_enum = static_cast<int>(a);
            float v = state.axes[glfw_enum];
            if (std::abs(v) > deadzone) {
                return v;
            }

            return 0.f;
        }
        bool get_button_state(gamepad_t g, button_t b) {
            GLFWgamepadstate state;
            glfwGetGamepadState(g, &state);
            int glfw_enum = static_cast<int>(b);
            return state.buttons[glfw_enum];
        }
    }
}
