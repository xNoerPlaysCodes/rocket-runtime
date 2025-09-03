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

        /// --- Extras ---
        bool undecorated = false;
        bool hidden = false;
        bool minimized = false;
        bool maximized = false;
        bool unfocused = false;
        bool topmost = false;
        /// @brief This requires very specific circumstances
        bool transparent = false;
        bool always_run = false;
        bool hidpi = false;
        bool interlacing = false;

        /// --- OpenGL Context ---
        /// @brief 4.3
        rocket::vec2i_t gl_version = { 4, 3 };
        bool gl_contextverifier = true;
    };

    enum class cursor_mode_t : int {
        normal = 0,
        hidden = 1
    };

    enum class platform_type_t {
        unknown = 0,
        windows,
        linux_wayland,
        linux_x11,
        macos_cocoa,
    };

    struct platform_t {
        /// @brief Windowing API
        std::string name = "UnknownWindowingAPI";
        /// @brief RocketGE Platform Name
        std::string rge_name = "UnknownWindowingAPI::UnknownPlatformType";
        /// @brief Operating System
        std::string os_name = "UnknownOperatingSystem";
        /// @brief Platform-Type Enum
        platform_type_t type = platform_type_t::unknown;
    };

    class window_t {
    private:
        /// @brief GLFW Implementation
        GLFWwindow *glfw_window = nullptr;

        rocket::vec2i_t size = { 0, 0 };
        std::string title = "Null";

        cursor_mode_t mode = cursor_mode_t::normal;
        windowflags_t flags;

        friend class renderer_2d;
        friend class renderer_3d;
    private:
        bool is_wayland();
    public:
        /// @brief Sets the size of the window
        void set_size(const rocket::vec2i_t& size);
        /// @brief Sets the title of the window
        void set_title(const std::string& title);

        /// @brief Registers a function to be called when the window is closed
        void register_on_close(std::function<void()>);

        /// @brief Sets the cursor mode
        void set_cursor_mode(cursor_mode_t);

        /// @brief Sets the cursor position
        void set_cursor_position(const rocket::vec2d_t& pos);

        /// @brief Polls events
        /// @note Call this at the end of main loop
        /// @note or window will not respond
        void poll_events();
    public:
        /// @brief Gets the title of the window
        std::string get_title() const;
        /// @brief Gets the size of the window
        rocket::vec2i_t get_size() const;

        /// @brief Gets the cursor mode
        cursor_mode_t get_cursor_mode();

        /// @brief Gets the cursor position
        rocket::vec2d_t get_cursor_position();

        /// @brief Checks if the window is running
        bool is_running() const;

        /// @brief Gets the current time
        double get_time() const;
    public:
        /// @brief Gets the current platform
        platform_t get_platform();
    public:
        /// @brief creates a new window
        window_t(const rocket::vec2i_t& size = { 800, 600 }, 
                const std::string& title = "rGE - Example Window", 
                windowflags_t flags = windowflags_t());
    public:
        /// @brief closes the window
        /// @brief called by destructor
        void close();
        ~window_t();
    };
}

#endif
