#include "../../include/rocket/io.hpp"
#include "rocket/runtime.hpp"
#include "rocket/types.hpp"
#include "util.hpp"

#include <GLFW/glfw3.h>

namespace rocket {
    namespace io {
        int scancode_by_key(keyboard_key key) {
            return glfwGetKeyScancode(static_cast<int>(key));
        }

        int key_by_scancode(int scancode) {
            static std::unordered_map<int, int> glfw_key_scancode;
            if (glfw_key_scancode.empty()) {
                for (int i = static_cast<int>(keyboard_key::first_key); i <= static_cast<int>(keyboard_key::last_key); ++i) {
                    glfw_key_scancode[glfwGetKeyScancode(i)] = i;
                }
            }

            if (glfw_key_scancode.find(scancode) != glfw_key_scancode.end()) {
                return glfw_key_scancode[scancode];
            }

            rocket::log_error("invalid key, no scancode for key was found", -1, "rocket::io::key_by_scancode", "fatal-to-function");
            return -1;
        }

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

        void simulate(keyboard_key key, keystate_t state) {
            key_event_t event;
            event.scancode = glfwGetKeyScancode(static_cast<int>(key));
            event.key = key;
            event.state = state;

            ::util::dispatch_event(event, true);
        }

        void simulate(mouse_button btn, keystate_t state, rocket::vec2d_t position) {
            mouse_event_t event;
            event.button = btn;
            event.state = state;
            event.position = position;

            ::util::dispatch_event(event, true);
        }

        void simulate(rocket::vec2d_t position, rocket::vec2d_t old_position) {
            if (old_position == rocket::vec2d_t { rocket::cst::io_mn_set_to_current_mpos, rocket::cst::io_mn_set_to_current_mpos }) {
                old_position = io::mouse_pos();
            }

            mouse_move_event_t event;
            event.old_pos = old_position;
            event.pos = position;

            ::util::dispatch_event(event, true);
        }
    }

    // IMMD IO
    namespace io {
        bool key_pressed(keyboard_key key) {
            return ::util::key_pressed(key);
        }

        bool key_down(keyboard_key key) {
            bool cur_actual = ::util::key_down(key);
            for (auto &kev : ::util::get_simulated_kevents()) {
                if (kev.key == key && kev.state.down()) {
                    return true;
                }
            }

            return cur_actual;
        }

        bool key_released(keyboard_key key) {
            bool cur_actual = ::util::key_released(key);
            for (auto &kev : ::util::get_simulated_kevents()) {
                if (kev.key == key && kev.state.released()) {
                    return true;
                }
            }
            return cur_actual;
        }

        bool key_state(keyboard_key key, keystate_t state) {
            if (state == keystate_t::make_down()) {
                return key_down(key);
            } else if (state == keystate_t::make_pressed()) {
                return key_pressed(key);
            } else if (state == keystate_t::make_released()) {
                return key_released(key);
            }
            rocket::log_error("[fixme] unimplemented key_state", -1, "rocket::io::key_state", "warning");
            return false;
        }

        bool mouse_pressed(mouse_button button) {
            bool cur_actual = ::util::mouse_pressed(button);
            for (auto &mev : ::util::get_simulated_mevents()) {
                if (mev.button == button && mev.state.pressed()) {
                    return true;
                }
            }
            return cur_actual;
        }

        bool mouse_down(mouse_button button) {
            bool cur_actual = ::util::mouse_down(button);
            for (auto &mev : ::util::get_simulated_mevents()) {
                if (mev.button == button && mev.state.down()) {
                    return true;
                }
            }
            return cur_actual;
        }

        bool mouse_released(mouse_button button) {
            bool cur_actual = ::util::mouse_released(button);
            for (auto &mev : ::util::get_simulated_mevents()) {
                if (mev.button == button && mev.state.released()) {
                    return true;
                }
            }
            return cur_actual;
        }

        bool mouse_state(mouse_button button, keystate_t state) {
            if (state.down()) {
                return ::util::mouse_down(button);
            } else if (state.pressed()) {
                return ::util::mouse_pressed(button);
            } else if (state.released()) {
                return ::util::mouse_released(button);
            }
            rocket::log_error("[fixme] unimplemented key_state", -1, "rocket::io::mouse_state", "warning");
            return false;
        }

        rocket::vec2d_t mouse_pos() {
            rocket::vec2d_t pos = ::util::mouse_pos();
            auto mmevs = ::util::get_simulated_mmevents();
            if (!mmevs.empty()) {
                return mmevs.back().pos;
            }
            return pos;
        }

        rocket::vec2f_t mouse_pos_f() {
            return {static_cast<float>(mouse_pos().x), static_cast<float>(mouse_pos().y)};
        }

        char get_formatted_char_typed() {
            return ::util::get_formatted_char_typed();
        }
    }

    namespace gpad {
        bool gpad_focused = true;
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
            rocket::log_error("[fixme] unimplemented", -1, "rocket::gpad::get_human_readable_name", "warning");
            return "Unknown";
        }
        std::string get_human_readable_name(axis_t, style_t) {
            rocket::log_error("[fixme] unimplemented", -1, "rocket::gpad::get_human_readable_name", "warning");
            return "Unknown";
        }

        std::string get_name(gamepad_t handle) {
            if (!is_available(handle)) {
                rocket::log_error("Gamepad not available with ID: " + std::to_string(handle), -1, "rocket::gpad::get_name", "warning");
                return "Unknown";
            }
            return glfwGetGamepadName(handle);
        }

        float get_axis_state(gamepad_t g, axis_t a, float deadzone) {
            if (!gpad_focused) {
                return -1.f;
            }
            GLFWgamepadstate state;
            glfwGetGamepadState(g, &state);
            int glfw_enum = static_cast<int>(a);
            float v = state.axes[glfw_enum];
            if (std::abs(v) > deadzone) {
                return v;
            }

            return -1.f;
        }
        bool get_button_state(gamepad_t g, button_t b) {
            if (!gpad_focused) {
                return false;
            }
            GLFWgamepadstate state;
            glfwGetGamepadState(g, &state);
            int glfw_enum = static_cast<int>(b);
            return state.buttons[glfw_enum];
        }

        void set_focused(bool focused) {
            gpad_focused = focused;
        }
    }
}
