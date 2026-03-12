#ifndef ROCKETGE__INTL_INTERNAL_TYPES_HPP
#define ROCKETGE__INTL_INTERNAL_TYPES_HPP

#include "lib/stb/stb_truetype.h"
#include <glm/glm.hpp>
#include <rocket/io.hpp>
#include <rocket/window.hpp>
#include <util.hpp>

#define MkFuncPtr0(ret_type, name) ret_type (*name)()
#define MkFuncPtr(ret_type, name, ...) ret_type (*name)(__VA_ARGS__)

namespace rocket {
    struct internal_cdata {
        stbtt_bakedchar a[96];
    };

    /// @brief Handle to a Native Window
    struct native_window_t {
        /// @brief Window Ptr
        void *w;
        /// @brief Window Backend Type
        window_backend_t backend;

        static native_window_t *bound;

        static void set_instance(native_window_t *window);
        static native_window_t *get_instance();
    };

    // [TODO] Implement
    namespace native_io {
        void            get_cursor_position(native_window_t *win, double *x, double *y);
        io::keystate_t  get_key_state(native_window_t *win, io::keyboard_key k);
        io::keystate_t  get_btn_state(native_window_t *win, io::mouse_button b);
    }

    class renderer_2d;
    class glfw_window_t;
    class asset_manager_t;

    struct renderer_2d_impl_t {
        renderer_2d *obj;
        glm::mat4 camera_transform = glm::mat4(1.0f);
    };

    struct glfw_window_impl_t {
        glfw_window_t *obj;
    };

    struct window_backend_i_impl_t {
        window_backend_i *obj;
        renderer_2d *bound_renderer2d = nullptr;
    };

    struct asset_manager_impl_t {
        asset_manager_t *obj;
    };
}

#endif//ROCKETGE__INTL_INTERNAL_TYPES_HPP
