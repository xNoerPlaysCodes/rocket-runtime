#include "rocket/window.hpp"
#include "window.hpp"

namespace rocket {
    std::function<void(rocket::window_backend_t)> cpl_init_callback;
    void window::cpl_init(window_backend_t bk) {
        switch (bk) {
            case rocket::window_backend_t::null:
                rocket::window::null_cpl_init(); break;
            case rocket::window_backend_t::glfw:
                rocket::window::glfw_cpl_init(); break;
            case rocket::window_backend_t::embedded:
                window::cpl_init(window_backend_t::null); break; // [FIXME] Add Embedded or remove it entirely
            case rocket::window_backend_t::android:
                rocket::window::android_cpl_init(); break;
            default: if (cpl_init_callback != nullptr) cpl_init_callback(bk); break;
        }
    }

    void window::register_cpl_init_callback(std::function<void(rocket::window_backend_t)> cb) {
        cpl_init_callback = cb;
    }
}
