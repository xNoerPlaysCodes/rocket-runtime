#ifndef ROCKETGE__WINDOW_HPP
#define ROCKETGE__WINDOW_HPP

#include "types.hpp"
#include "macros.hpp"
#include "asset.hpp"

#include <functional>
#include <rocket/glfnldr.hpp>
#include <rocket/io.hpp>
#include <string>
#include <rocket/window_helpers.hpp>
#include "modularity/window_backend.hpp"

namespace rocket {
    enum class window_backend_t : int {
        null = 0,
        glfw = 1,
        embedded = 2,
        android = 3,

        custom_1 = 100,
        custom_2 = 101,
        custom_3 = 102,
        custom_4 = 103,
        custom_5 = 104
    };

    struct native_window_t;

    namespace window {
        /// @brief Force a specific platform to use in GLFW/Backend
        /// @note Must be called BEFORE constructor of all windows
        void set_forced_platform(platform_type_t type);

        /// @brief Initialize the CPL Windowing Library before-hand
        void cpl_init(window_backend_t backend = window_backend_t::glfw);

        /// @brief Register CPL Initialization Callback
        void register_cpl_init_callback(std::function<void(window_backend_t bk)> fn);
    }

    struct glfw_window_impl_t;

    class glfw_window_t : public window_backend_i {
    private:
        /// @brief GLFW Implementation
        native_window_t *glfw_window = nullptr;

        io::keystate_t keys[348 + 1] = {};
        io::keystate_t buttons[7 + 1] = {};
        bool shown = false;

        glfw_window_impl_t *impl = nullptr;
    protected:
        void swap_buffers() const override;
    public:
        /// @brief Sets the size of the window
        void set_size(const rocket::vec2i_t& size) override;
        /// @brief Sets the title of the window
        void set_title(const std::string& title) override;

        /// @brief Registers a function to be called when the window is closed
        void register_on_close(std::function<void()>) override;

        /// @brief Sets the cursor mode
        void set_cursor_mode(cursor_mode_t) override;

        /// @brief Sets the cursor position
        void set_cursor_position(const rocket::vec2d_t& pos) const override;

        /// @brief Polls events
        /// @note Call this at the end of main loop
        /// @note or window will not respond
        void poll_events() override;
    public:
        /// @brief Gets the title of the window
        std::string get_title() const override;
        /// @brief Gets the size of the window
        rocket::vec2i_t get_size() const override;

        /// @brief Gets the cursor mode
        cursor_mode_t get_cursor_mode() const override;

        /// @brief Gets the cursor position
        rocket::vec2d_t get_cursor_position() const override;

        /// @brief Checks if the window is running
        bool is_running() const override;

        /// @brief Gets the current time
        double get_time() const override;
    public:
        /// @brief Gets the current window state
        window_state_t get_window_state() const override;

        /// @brief Sets the current window state
        void set_window_state(window_state_t state) const override;
    public:
        /// @brief Gets the window icon
        std::shared_ptr<rocket::texture_t> get_icon() const override;

        /// @brief Sets the window icon
        void set_icon(std::shared_ptr<rocket::texture_t> icon) override;
    public:
        /// @brief Sets the current VSync state
        void set_vsync(bool vsync) override;
    public:
        /// @brief Gets the current platform
        platform_t get_platform() const override;
    public:
        /// @brief creates a new window
        glfw_window_t(const rocket::vec2i_t& size = { 800, 600 }, 
                const std::string& title = "rGE - Example Window", 
                windowflags_t flags = windowflags_t());
    public:
    public:
        /// @brief closes the window
        /// @brief called by destructor
        void close() override;
        ~glfw_window_t();
    };

    class null_window_t : public window_backend_i {
    protected:
        void swap_buffers() const override;
    public:
        void set_size(const rocket::vec2i_t& size) override;
        void set_title(const std::string& title) override;

        void register_on_close(std::function<void()>) override;

        void set_cursor_mode(cursor_mode_t) override;

        void set_cursor_position(const rocket::vec2d_t& pos) const override;

        void poll_events() override;
    public:
        std::string get_title() const override;
        rocket::vec2i_t get_size() const override;

        cursor_mode_t get_cursor_mode() const override;

        rocket::vec2d_t get_cursor_position() const override;

        bool is_running() const override;

        double get_time() const override;
    public:
        window_state_t get_window_state() const override;

        void set_window_state(window_state_t state) const override;
    public:
        std::shared_ptr<rocket::texture_t> get_icon() const override;

        void set_icon(std::shared_ptr<rocket::texture_t> icon) override;
    public:
        void set_vsync(bool vsync) override;
    public:
        platform_t get_platform() const override;
    public:
        null_window_t(const rocket::vec2i_t& size = { 800, 600 }, 
                const std::string& title = "rGE - Example Window", 
                windowflags_t flags = windowflags_t());
    public:
    public:
        void close() override;
        ~null_window_t();
    };
    
    struct android_app_impl_t;

    class android_app_t : public window_backend_i {
    private:
        io::keystate_t keys[348 + 1] = {};
        io::keystate_t buttons[7 + 1] = {};
        bool shown = false;

        bool running = false;
        bool surface_alive = false;

        android_app_impl_t *impl = nullptr;
    protected:
        void swap_buffers() const override;
    private:
        void create_surface();
        void destroy_surface();
        void resume_surface();
        void pause_surface();
    public:
        /// @brief Sets the size of the window
        void set_size(const rocket::vec2i_t& size) override;
        /// @brief Sets the title of the window
        void set_title(const std::string& title) override;

        /// @brief Registers a function to be called when the window is closed
        void register_on_close(std::function<void()>) override;

        /// @brief Sets the cursor mode
        void set_cursor_mode(cursor_mode_t) override;

        /// @brief Sets the cursor position
        void set_cursor_position(const rocket::vec2d_t& pos) const override;

        /// @brief Polls events
        /// @note Call this at the end of main loop
        /// @note or window will not respond
        void poll_events() override;
    public:
        /// @brief Gets the title of the window
        std::string get_title() const override;
        /// @brief Gets the size of the window
        rocket::vec2i_t get_size() const override;

        /// @brief Gets the cursor mode
        cursor_mode_t get_cursor_mode() const override;

        /// @brief Gets the cursor position
        rocket::vec2d_t get_cursor_position() const override;

        /// @brief Checks if the window is running
        bool is_running() const override;

        /// @brief Gets the current time
        double get_time() const override;
    public:
        /// @brief Gets the current window state
        window_state_t get_window_state() const override;

        /// @brief Sets the current window state
        void set_window_state(window_state_t state) const override;
    public:
        /// @brief Gets the window icon
        std::shared_ptr<rocket::texture_t> get_icon() const override;

        /// @brief Sets the window icon
        void set_icon(std::shared_ptr<rocket::texture_t> icon) override;
    public:
        /// @brief Sets the current VSync state
        void set_vsync(bool vsync) override;
    public:
        /// @brief Gets the current platform
        platform_t get_platform() const override;
    public:
        /// @brief creates a new window
        android_app_t(const rocket::vec2i_t& size = { 800, 600 }, 
                const std::string& title = "rGE - Example Window", 
                windowflags_t flags = windowflags_t());
    public:
    public:
        /// @brief closes the window
        /// @brief called by destructor
        void close() override;
        ~android_app_t();
    };

    /// @brief Alias to Native Platform Window
#ifdef ROCKETGE__Platform_Desktop
    using window_t = glfw_window_t;
#else
    using window_t = android_app_t;
#endif
}

#endif
