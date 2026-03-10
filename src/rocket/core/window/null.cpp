#include "rocket/window.hpp"
#include "window.hpp"
#include <rocket/modularity/window_backend.hpp>
#include <rocket/runtime.hpp>
#include <internal_types.hpp>

namespace rocket {
    void window::null_cpl_init() {
    }

    platform_t null_window_t::get_platform() const {
        platform_t platform;
        platform.name = "Null";
        platform.rge_name = std::string("Null::Null");
        platform.os_name = "Null";
        return platform;
    }

    void null_window_t::set_vsync(bool vsync) {
    }

    null_window_t::null_window_t(const rocket::vec2i_t& size,
            const std::string& title,
            windowflags_t flags) {
        this->handle = new native_window_t;
        this->handle->w = nullptr;
        this->handle->backend = window_backend_t::null;
        native_window_t::set_instance(this->handle);

        this->size = size;
        this->title = title;
        this->flags = flags;
    }

    void null_window_t::set_window_state(window_state_t state) const {
    }

    void null_window_t::set_icon(std::shared_ptr<rocket::texture_t> texture) {
    }

    void null_window_t::set_cursor_mode(cursor_mode_t m) {
    }

    void null_window_t::set_cursor_position(const rocket::vec2d_t& pos) const {
    }

    void null_window_t::swap_buffers() const {
    }

    void null_window_t::set_size(const rocket::vec2i_t& size) {
        this->size = size;
    }

    void null_window_t::set_title(const std::string& title) {
        this->title = title;
    }

    void null_window_t::register_on_close(std::function<void()> listener) {
    }

    void null_window_t::poll_events() {
    }

    std::string null_window_t::get_title() const {
        return this->title;
    }

    rocket::vec2i_t null_window_t::get_size() const {
        return this->size;
    }

    rocket::vec2d_t null_window_t::get_cursor_position() const {
        return {0,0};
    }

    bool null_window_t::is_running() const {
        return true;
    }

    double null_window_t::get_time() const {
        return 0;
    }

    window_state_t null_window_t::get_window_state() const {
        return {};
    }

    std::shared_ptr<texture_t> null_window_t::get_icon() const {
        return nullptr;
    }

    cursor_mode_t null_window_t::get_cursor_mode() const {
        return {};
    }

    void null_window_t::close() {
        rocket::log("Window closed", "null_window_t", "destructor", "info");

        if (this->handle != nullptr) {
            delete this->handle;
            this->handle = nullptr;
        }
    }

    null_window_t::~null_window_t() {
        destructor_called = true;
        close();
        destructor_called = false;
    }
}
