#pragma once

#include <rocket/glfnldr.hpp>
#include <rocket/types.hpp>
#include <functional>

namespace rocket {
    constexpr int fps_uncapped = -1; // [[TODO]] Put this in rocket::cst
    struct renderer_flags_t {
        bool share_renderer_as_global = true;
        /// @brief Show the splash screen
        bool show_splash = true;
        /// @brief (Advanced) Change Glfnldr backend if available
        /// @note List available backends with glfnldr::get_backends()
        glfnldr::backend_t glfnldr_backend = ROCKETGE__GLFNLDR_BACKEND_ENUM;
    };
    enum class render_mode_t {
        texture_filter_none,
        camera,
    };

    struct graphics_settings_t {
        bool viewport_culling = true;
    };

    enum class renderer_backend_t {
        null,
        opengl,
        vulkan,
    };

    /// @brief API-agnostic Framebuffer Object
    struct framebuffer_t {
    private:
        api_object_t fbo = ROCKETGE__InvalidNumber;
        rocket::vec2f_t size = {};
        int binding = -1;
    public:
        /// @brief Binds and returns a texture unit
        /// @note between [0-32]
        int bind();

        /// @brief Frees binding
        /// @note Called automatically on destructor
        void free_binding();
    public:
        framebuffer_t(rocket::vec2f_t size);
        ~framebuffer_t();
    };

    class renderer_2d_i;

    struct render_cache_t {
    private:
        api_object_t fbo = ROCKETGE__InvalidNumber;
        std::function<void(rocket::renderer_2d_i *ren)> draw = nullptr;
        friend class renderer_2d_i;
        friend class opengl_renderer_2d;
        friend class vulkan_renderer_2d;
    public:
        api_object_t get_texture() const;
    };
}
