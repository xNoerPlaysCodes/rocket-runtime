#ifndef ROCKETGE__WINDOW_HPP
#define ROCKETGE__WINDOW_HPP

#include "types.hpp"

#include <GLFW/glfw3.h>
#include <functional>
#include <string>

namespace rocket {
    struct windowflags_t {
        bool fullscreen = false;
        bool vsync = false;
        bool resizable = true;
        int msaa_samples = 0;
    };

    enum class cursor_mode_t : int {
        normal = 0,
        hidden = 1
    };

    class window_t {
    private:
        GLFWwindow *glfw_window = nullptr;

        rocket::vec2i_t size = { 1, 1 };
        std::string title = "Null";

        cursor_mode_t mode = cursor_mode_t::normal;

        friend class renderer_2d;
        friend class renderer_3d;
    public:
        void set_size(const rocket::vec2i_t& size);
        void set_title(const std::string& title);

        void register_on_close(std::function<void()>);

        void set_cursor_mode(cursor_mode_t);

        void set_cursor_position(const rocket::vec2d_t& pos);

        void poll_events();
    public:
        std::string get_title() const;
        rocket::vec2i_t get_size() const;

        cursor_mode_t get_cursor_mode();

        rocket::vec2d_t get_cursor_position();

        bool is_wayland();

        bool is_running() const;
    public:
        /// @brief creates a new window
        window_t(const rocket::vec2i_t& size = { 800, 600 }, 
                const std::string& title = "RocketGE - Example Window", 
                windowflags_t flags = windowflags_t());
    public:
        /// @brief closes the window
        /// @brief called by destructor
        void close();
        ~window_t();
    };
}

#endif
