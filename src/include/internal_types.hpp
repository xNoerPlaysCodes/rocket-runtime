#ifndef ROCKETGE__INTL_INTERNAL_TYPES_HPP
#define ROCKETGE__INTL_INTERNAL_TYPES_HPP

#include "lib/stb/stb_truetype.h"
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <rocket/io.hpp>
#include <rocket/renderer.hpp>
#include <rocket/rgl.hpp>
#include <rocket/window.hpp>
#include <stack>
#include <util.hpp>

#define MkFuncPtr0(ret_type, name) ret_type (*name)()
#define MkFuncPtr(ret_type, name, ...) ret_type (*name)(__VA_ARGS__)

#ifdef ROCKETGE__Platform_Android
#include <EGL/egl.h>
#endif

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

    class renderer_2d_i;
    class glfw_window_t;
    class asset_manager_t;

    struct renderer_2d_impl_t {
        renderer_2d_i *obj;
        std::vector<std::unique_ptr<render_cache_t>> render_caches;
        std::stack<render_cache_t*> render_cache_use_stack;
        glm::mat4 camera_transform = glm::mat4(1.0f);
        std::atomic<api_object_t> current_object_handle = 0;
    };

    using _GLuint = uint32_t;
    using _GLint = int32_t;

    enum class gl_object_type_t {
        texture,
    };

    struct gl_object_t { // 8 bytes
        gl_object_type_t type;
        _GLuint value;
    };

    struct opengl_renderer_2d_impl_t {
        std::unordered_map<api_object_t, gl_object_t> objects;
    };

    enum class vk_object_type_t {
        texture,
        // anything here
    };

    struct vk_object_t { // 24 bytes
        vk_object_type_t type;
        void *value;
        void *additional; // Idk is this fine?
                          // it could be struct vk_texture_t,
                          // with all the necessary stuffs
    };

    struct vulkan_renderer_2d_impl_t {
        std::unordered_map<api_object_t, vk_object_t> objects;
    };

    struct glfw_window_impl_t {
        glfw_window_t *obj;
    };

    struct window_backend_i_impl_t {
        window_backend_i *obj;
        renderer_2d_i *bound_renderer2d = nullptr;
        util::timer_t init_timer;
    };

    struct asset_manager_impl_t {
        asset_manager_t *obj;
    };

    struct android_app_impl_t {
        android_app_t *obj;
        std::function<void()> on_close = nullptr;
#ifdef ROCKETGE__Platform_Android
        EGLDisplay display = EGL_NO_DISPLAY;
        EGLSurface surface = EGL_NO_SURFACE;
        EGLContext context = EGL_NO_CONTEXT;
        EGLConfig config   = nullptr;
#endif
    };
}

#endif//ROCKETGE__INTL_INTERNAL_TYPES_HPP
