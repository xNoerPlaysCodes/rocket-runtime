#include "../../include/rocket/io.hpp"
#include "rocket/runtime.hpp"
#include "rocket/types.hpp"
#include "util.hpp"

#include <GLFW/glfw3.h>
#include <SDL2/SDL_gamecontroller.h>
#include <SDL2/SDL_haptic.h>
#include <SDL2/SDL_joystick.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>

// if anyone has a better idea please do make a PR
#include <SDL2/SDL.h>

namespace std {
    template <>
    struct hash<std::pair<rocket::gpad::button_t, rocket::gpad::style_t>> {
        std::size_t operator()(const std::pair<rocket::gpad::button_t, rocket::gpad::style_t>& p) const noexcept {
            std::size_t h1 = static_cast<std::size_t>(p.first);
            std::size_t h2 = static_cast<std::size_t>(p.second);
            return h1 ^ (h2 << 1); // simple hash combine
        }
    };
}

namespace std {
    template <>
    struct hash<std::pair<rocket::gpad::axis_t, rocket::gpad::style_t>> {
        std::size_t operator()(const std::pair<rocket::gpad::axis_t, rocket::gpad::style_t>& p) const noexcept {
            std::size_t h1 = static_cast<std::size_t>(p.first);
            std::size_t h2 = static_cast<std::size_t>(p.second);
            return h1 ^ (h2 << 1); // simple hash combine
        }
    };
}

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

            rocket::log("invalid key, no scancode for key was found", "rocket::io", "key_by_scancode", "error");
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

        rocket::fbounding_box mouse_bbox() {
            return { {io::mouse_pos_f().x, io::mouse_pos_f().y}, {1, 1} };
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
            rocket::log("[fixme] unimplemented key_state", "rocket::io", "key_state", "fixme");
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
            rocket::log("[fixme] unimplemented key_state", "rocket::io", "mouse_state", "fixme");
            return false;
        }

        rocket::vec2d_t mouse_pos() {
            rocket::vec2d_t pos = ::util::mouse_pos();
            pos.x = std::max(0.0, pos.x);
            pos.y = std::max(0.0, pos.y);
            auto mmevs = ::util::get_simulated_mmevents();
            if (!mmevs.empty()) {
                return mmevs.back().pos;
            }

            return pos;
        }

        rocket::vec2f_t mouse_pos_f() {
            auto mpos = mouse_pos();
            return {static_cast<float>(mpos.x), static_cast<float>(mpos.y)};
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
                rocket::log("Gamepad not available with ID: " + std::to_string(id), "rocket::gpad", "get_handle", "warning");
                return 255;
            }

            return static_cast<gamepad_t>(id);
        }

        std::vector<gamepad_t> get_available_gamepads() {
            std::vector<gamepad_t> gpads;
            for (int i = 0; i <= GLFW_JOYSTICK_LAST; ++i) {
                if (is_available(i)) gpads.push_back(static_cast<gamepad_t>(i));
            }

            return gpads;
        }

        std::string get_human_readable_name(button_t button, style_t style) {
            using enum button_t;
            const std::unordered_map<std::pair<button_t, style_t>, std::string> button_names = {
                // Xbox
                { {x, style_t::xbox}, "X" },
                { {y, style_t::xbox}, "Y" },
                { {a, style_t::xbox}, "A" },
                { {b, style_t::xbox}, "B" },
                { {view, style_t::xbox}, "View" },
                { {menu, style_t::xbox}, "Menu" },
                { {left_bumper, style_t::xbox}, "LB" },
                { {right_bumper, style_t::xbox}, "RB" },
                { {guide, style_t::xbox}, "Xbox" },
                { {left_stick, style_t::xbox}, "LS" },
                { {right_stick, style_t::xbox}, "RS" },
                { {dpad_up, style_t::xbox}, "D-Pad Up" },
                { {dpad_down, style_t::xbox}, "D-Pad Down" },
                { {dpad_left, style_t::xbox}, "D-Pad Left" },
                { {dpad_right, style_t::xbox}, "D-Pad Right" },

                // DualShock
                { {square, style_t::dualshock}, "Square" },
                { {triangle, style_t::dualshock}, "Triangle" },
                { {cross, style_t::dualshock}, "Cross" },
                { {circle, style_t::dualshock}, "Circle" },
                { {share, style_t::dualshock}, "Share" },
                { {option, style_t::dualshock}, "Option" },
                { {l1, style_t::dualshock}, "L1" },
                { {r1, style_t::dualshock}, "R1" },
                { {guide, style_t::dualshock}, "PS" },
                { {left_stick, style_t::dualshock}, "L3" },
                { {right_stick, style_t::dualshock}, "R3" },
                { {dpad_up, style_t::dualshock}, "D-Pad Up" },
                { {dpad_down, style_t::dualshock}, "D-Pad Down" },
                { {dpad_left, style_t::dualshock}, "D-Pad Left" },
                { {dpad_right, style_t::dualshock}, "D-Pad Right" },
            };

            auto it = button_names.find({button, style});
            if (it != button_names.end()) {
                return it->second;
            }
            return "Unknown";
        }
        std::string get_human_readable_name(axis_t axis, style_t style) {
            using enum axis_t;
            using enum style_t;
            const std::unordered_map<std::pair<axis_t, style_t>, std::string> axis_names = {
                { {left_x, xbox}, "Left Stick X" },
                { {left_y, xbox}, "Left Stick Y" },
                { {right_x, xbox}, "Right Stick X" },
                { {right_y, xbox}, "Right Stick Y" },
                { {left_trigger, xbox}, "Left Trigger" },
                { {right_trigger, xbox}, "Right Trigger" },

                { {left_x, dualshock}, "Left Stick X" },
                { {left_y, dualshock}, "Left Stick Y" },
                { {right_x, dualshock}, "Right Stick X" },
                { {right_y, dualshock}, "Right Stick Y" },
                { {l2, dualshock}, "L2" },
                { {r2, dualshock}, "R2" }
            };

            auto it = axis_names.find({axis, style});
            if (it != axis_names.end()) {
                return it->second;
            }
            return "Unknown";
        }

        std::string get_name(gamepad_t handle) {
            if (!is_available(handle)) {
                rocket::log("Gamepad not available with ID: " + std::to_string(handle), "rocket::gpad", "get_name", "warning");
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

        void sdl_vibr_init() {
            static bool is_init = false;
            if (!is_init) {
                SDL_Init(SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK);
                is_init = true;
            }
        }


        void set_vibration(gamepad_t gp, float strength, float duration_ms) {
            sdl_vibr_init();
            static std::unordered_map<gamepad_t, SDL_GameController*> controllers;
            static std::unordered_map<SDL_GameController*, SDL_Haptic*> haptics;

            SDL_GameController* gc = nullptr;
            SDL_Haptic* haptic = nullptr;

            auto itc = controllers.find(gp);
            if (itc != controllers.end()) {
                gc = itc->second;
                haptic = haptics[gc];
            } else {
                SDL_JoystickGUID tgt = SDL_JoystickGetGUIDFromString(glfwGetJoystickGUID(gp));
                int n = SDL_NumJoysticks();
                for (int i = 0; i < n; ++i) {
                    SDL_Joystick* j = SDL_JoystickOpen(i);
                    if (!j) continue;
                    SDL_JoystickGUID guid = SDL_JoystickGetGUID(j);
                    if (SDL_memcmp(&guid, &tgt, sizeof(guid)) == 0) {
                        gc = SDL_GameControllerOpen(i);
                        haptic = SDL_HapticOpenFromJoystick(j);
                        if (haptic) SDL_HapticRumbleInit(haptic);

                        controllers[gp] = gc;
                        haptics[gc] = haptic;
                        break;
                    }
                }
            }

            if (gc == nullptr || haptic == nullptr)
                return;

            SDL_HapticRumblePlay(haptic, strength, static_cast<int>(duration_ms));
        }
    }
}
