#ifndef ROCKETGE__IO_HPP
#define ROCKETGE__IO_HPP

#include "types.hpp"
#include <functional>
#include <string>
namespace rocket {
    namespace io {
        enum class mouse_button : int {
            left = 0,
            right = 1,
            middle = 2,
        };

        enum class keyboard_key : int {
            unknown = -1,

            first_key = 32,

            space = 32, apostrophe = 39, comma = 44, minus = 45, period = 46, slash = 47,
            semicolon = 59, equal = 61,
            left_bracket = 91, backslash = 92, right_bracket = 93, grave_accent = 96,

            digit_0 = 48, digit_1 = 49, digit_2 = 50, digit_3 = 51, digit_4 = 52,
            digit_5 = 53, digit_6 = 54, digit_7 = 55, digit_8 = 56, digit_9 = 57,

            a = 65, b = 66, c = 67, d = 68, e = 69, f = 70, g = 71, h = 72, i = 73, j = 74,
            k = 75, l = 76, m = 77, n = 78, o = 79, p = 80, q = 81, r = 82, s = 83, t = 84,
            u = 85, v = 86, w = 87, x = 88, y = 89, z = 90,

            world_1 = 161, world_2 = 162,

            escape = 256, enter = 257, tab = 258, backspace = 259, insert = 260, delete_key = 261,
            right = 262, left = 263, down = 264, up = 265, page_up = 266, page_down = 267,
            home = 268, end = 269, caps_lock = 280, scroll_lock = 281, num_lock = 282,
            print_screen = 283, pause = 284,

            f1 = 290, f2 = 291, f3 = 292, f4 = 293, f5 = 294, f6 = 295,
            f7 = 296, f8 = 297, f9 = 298, f10 = 299, f11 = 300, f12 = 301,
            f13 = 302, f14 = 303, f15 = 304, f16 = 305, f17 = 306, f18 = 307,
            f19 = 308, f20 = 309, f21 = 310, f22 = 311, f23 = 312, f24 = 313, f25 = 314,

            kp_0 = 320, kp_1 = 321, kp_2 = 322, kp_3 = 323, kp_4 = 324,
            kp_5 = 325, kp_6 = 326, kp_7 = 327, kp_8 = 328, kp_9 = 329,
            kp_decimal = 330, kp_divide = 331, kp_multiply = 332, kp_subtract = 333,
            kp_add = 334, kp_enter = 335, kp_equal = 336,

            left_shift = 340, left_control = 341, left_alt = 342, left_super = 343,
            right_shift = 344, right_control = 345, right_alt = 346, right_super = 347,
            menu = 348,

            last_key = 348
        };

        /// @brief Keystate
        struct keystate_t {
            /// @modify Do not modify
            bool current = false;
            /// @modify Do not modify
            bool previous = false;

            /// @brief Checks if the key is pressed
            bool pressed()  const;
            /// @brief Checks if the key is released
            bool released() const;
            /// @brief Checks if the key is down
            bool down()     const;

            static keystate_t make_pressed();
            static keystate_t make_released();
            static keystate_t make_down();

            bool operator==(const keystate_t& other) const { return current == other.current && previous == other.previous; }
        };

        struct key_event_t {
            keyboard_key key;
            keystate_t state;
            int scancode = 0;
        };

        struct mouse_event_t {
            mouse_button button;
            keystate_t state;
        };

        struct mouse_move_event_t {
            rocket::vec2d_t old_pos = { 0, 0 };
            rocket::vec2d_t pos = { 0, 0 };
        };

        struct scroll_offset_event_t {
            rocket::vec2d_t offset = { 0, 0 };
        };

        /// @brief Add a key event listener
        void add_listener(std::function<void(key_event_t)>);
        /// @brief Add a mouse event listener
        void add_listener(std::function<void(mouse_event_t)>);
        /// @brief Add a mouse move event listener
        void add_listener(std::function<void(mouse_move_event_t)>);
        /// @brief Add a scroll offset event listener
        void add_listener(std::function<void(scroll_offset_event_t)>);

        // IMMD IO

        /// @brief Checks if a key is down
        bool key_down(keyboard_key key);
        /// @brief Checks if a key is pressed
        bool key_pressed(keyboard_key key);
        /// @brief Checks if a key is released
        bool key_released(keyboard_key key);

        /// @brief Checks if a key is a particular state
        bool key_state(keyboard_key key, keystate_t state);

        /// @brief Get mouse position
        rocket::vec2d_t mouse_pos();
        /// @brief Get mouse position... in float
        rocket::vec2f_t mouse_pos_f();

        /// @brief Get mouse bounding box
        rocket::fbounding_box mouse_bbox();

        /// @brief Checks if a mouse button is down
        bool mouse_down(mouse_button button);
        /// @brief Checks if a mouse button is pressed
        bool mouse_pressed(mouse_button button);
        /// @brief Checks if a mouse button is released
        bool mouse_released(mouse_button button);

        /// @brief Checks if a mouse button is a particular state
        bool mouse_state(mouse_button button, keystate_t state);

        /// @brief Get char typed (formatted)
        char get_formatted_char_typed();
    }

    // Controller (Referred as Gamepad) Input
    namespace gpad {
        /// @brief Gamepad Buttons
        enum class button_t : int {
            bottom = 0,
            a = 0,
            cross = 0,

            right = 1,
            b = 1,
            circle = 1,

            left = 2,
            x = 2,
            square = 2,

            top = 3,
            y = 3,
            triangle = 3,

            left_bumper = 4,
            lb = 4,
            l1 = 4,

            right_bumper = 5,
            rb = 5,
            r1 = 5,

            start = 7,
            option = 7,

            guide = 8,
            xbox = 8,
            ps = 8,
            home = 8,

            left_stick = 9,
            l3 = 9,

            right_stick = 10,
            r3 = 10,

            dpad_up = 11,
            dpad_right = 12,
            dpad_down = 13,
            dpad_left = 14,
        };

        enum class axis_t {
            left_x = 0,
            left_y = 1,

            right_x = 2,
            right_y = 3,

            left_trigger = 4,
            l2 = 4,
            lt = 4,

            right_trigger = 5,
            r2 = 5,
            rt = 5,
        };

        enum class style_t : int {
            generic = 0,
            xbox,
            dualshock
        };

        using gamepad_t = uint8_t;

        bool is_available(int id = 0);
        gamepad_t get_handle(int id = 0);

        std::string get_human_readable_name(button_t, style_t);
        std::string get_human_readable_name(axis_t, style_t);

        float get_axis_state(gamepad_t, axis_t, float deadzone = 0.1f);
        bool get_button_state(gamepad_t, button_t);
    }
}

#endif
