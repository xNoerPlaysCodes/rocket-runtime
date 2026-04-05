#ifndef ROCKETGE__WINDOW_HELPERS_HPP
#define ROCKETGE__WINDOW_HELPERS_HPP

#include <rocket/renderer_helpers.hpp>
#include <rocket/types.hpp>
#include <rocket/macros.hpp>
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

        /// @brief Get the monitor with the cursor on it
        /// @note Must call window_t::cpl_init() before-hand
        static monitor_t with_cursor();
        /// @brief Load a monitor by index
        /// @param idx Default = 0 -> Primary Monitor
        /// @note Must call window_t::cpl_init() before-hand
        static monitor_t of(int idx = 0);
        /// @brief Get the amount of monitors
        /// @note Must call window_t::cpl_init() before-hand
        static int get_count();
    };

    struct graphics_api_context_t {
        rocket::vec2i_t version = {};
        bool debug_context = false;
        renderer_backend_t backend = renderer_backend_t::opengl;
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
        bool hidpi = false;
        bool interlacing = false;

        /// --- Graphics Context ---
        graphics_api_context_t graphics_ctx;

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
        android
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
}

#endif//ROCKETGE__WINDOW_HELPERS_HPP
