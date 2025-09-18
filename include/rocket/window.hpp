#ifndef ROCKETGE__WINDOW_HPP
#define ROCKETGE__WINDOW_HPP

#include "types.hpp"
#include "macros.hpp"
#include "asset.hpp"

#include <GLFW/glfw3.h>
#include <functional>
#include <string>

namespace rocket {
    /// @brief Monitor Representation Struct
    struct monitor_t {
        //  (with cursor = -1)
        int idx = -1;
        vec2i_t size = { 0, 0 };
        int redbits = 0;
        int greenbits = 0;
        int bluebits = 0;
        /// @brief The refresh rate, in Hz
        int refreshrate = 0;

        static monitor_t with_cursor();
        /// @brief Load a monitor by index
        /// @param idx Default = 0 -> Primary Monitor
        static monitor_t of(int idx = 0);
        /// @brief Get the amount of monitors
        static int get_count();
    };

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
        /// @brief 0-1 Window Framebuffer Opacity
        /// @note 0 means Fully Transparent
        /// @note 1 means Fully Opaque
        float opacity = 1;
        bool always_run = false;
        bool hidpi = false;
        bool interlacing = false;

        /// --- OpenGL Context ---
        /// @brief 4.3
        rocket::vec2i_t gl_version = { 4, 3 };
        bool gl_contextverifier = true;

        /// --- Advanced ---
        std::string window_class_name = ROCKETGE__PlatformSpecific_Linux_AppClassNameOrID;
    };

    struct window_state_t {
        /// @brief Is the window focused
        bool focused = false;

        /// @brief Is the window visible
        bool visible = false;

        /// @brief Is the window iconified
        bool iconified = false;

        /// @brief Is the window maximized
        bool maximized = false;

        /// @brief Is the window floating
        bool floating = false;

        /// @brief Is the cursor hovering over the window content
        ///         area DIRECTLY
        bool hovering = false;
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

        std::shared_ptr<rocket::texture_t> icon = nullptr;
        bool destructor_called = false;

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
        /// @brief Gets the current window state
        window_state_t get_window_state();

        /// @brief Sets the current window state
        void set_window_state(window_state_t state);
    public:
        /// @brief Gets the window icon
        std::shared_ptr<rocket::texture_t> get_icon();

        /// @brief Sets the window icon
        void set_icon(std::shared_ptr<rocket::texture_t> icon);
    public:
        /// @brief Gets the current platform
        platform_t get_platform();
    public:
        /// @brief creates a new window
        window_t(const rocket::vec2i_t& size = { 800, 600 }, 
                const std::string& title = "rGE - Example Window", 
                windowflags_t flags = windowflags_t());
    public:
        /// @brief Force a specific platform to use in GLFW/Backend
        /// @note Must be called BEFORE constructor of all windows
        static void set_forced_platform(platform_type_t type);
    public:
        /// @brief closes the window
        /// @brief called by destructor
        void close();
        ~window_t();
    };
}

#endif
