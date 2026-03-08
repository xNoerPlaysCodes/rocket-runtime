#ifndef ROCKETGE__INT_MODULARY_WINDOW_BACKEND_HPP
#define ROCKETGE__INT_MODULARY_WINDOW_BACKEND_HPP

#include <rocket/types.hpp>
#include <functional>
#include <rocket/window_helpers.hpp>
#include "rocket/asset.hpp"
#include <rocket/glfnldr.hpp>
#include <string>

namespace rgl {
    std::vector<std::string> init_gl(rocket::vec2f_t viewport_size, glfnldr::backend_t bkend);
}

namespace rocket {
    struct native_window_t;
    class window_backend_i {
    public:
        native_window_t *handle;
        rocket::vec2i_t size = { 0, 0 };
        std::string title = "Null";

        cursor_mode_t mode = cursor_mode_t::normal;
        windowflags_t flags;

        std::shared_ptr<rocket::texture_t> icon = nullptr;
        bool destructor_called = false;

        friend class renderer_2d;
        friend class renderer_3d;
        friend std::vector<std::string> rgl::init_gl(rocket::vec2f_t viewport_size, glfnldr::backend_t);
    protected:
        virtual void swap_buffers() const = 0;
    public:
        virtual ~window_backend_i() = default;
    public:
        virtual void set_size(const rocket::vec2i_t &size) = 0;
        virtual void set_title(const std::string &title) = 0;
        virtual void register_on_close(std::function<void()> fn) = 0;
        virtual void set_cursor_mode(cursor_mode_t mode) = 0;
        virtual void set_cursor_position(const rocket::vec2d_t &pos) const = 0;
    public:
        virtual std::string get_title() const = 0;
        virtual rocket::vec2i_t get_size() const = 0;
        virtual cursor_mode_t get_cursor_mode() const = 0;
        virtual rocket::vec2d_t get_cursor_position() const = 0;
        virtual bool is_running() const = 0;
        virtual double get_time() const = 0;
    public:
        virtual window_state_t get_window_state() const = 0;
        virtual void set_window_state(window_state_t state) const = 0;
        virtual platform_t get_platform() const = 0;
    public:
        virtual void set_vsync(bool) = 0;
    public:
        virtual void poll_events() = 0;
    public:
        virtual std::shared_ptr<rocket::texture_t> get_icon() const = 0;
        virtual void set_icon(std::shared_ptr<rocket::texture_t> icon) = 0;
    public:
        virtual void close() = 0;
    };}

#endif//ROCKETGE__INT_MODULARY_WINDOW_BACKEND_HPP
