#pragma once

#include <functional>
#include <rocket/glfnldr.hpp>

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

    class renderer_2d_i;

    struct render_cache_t {
    private:
        void *fbo;
        // rgl::fbo_t fbo;
        friend class renderer_2d_i;
    public:
        std::function<void(rocket::renderer_2d_i *ren)> draw = nullptr;
    public:
        /// @brief DO NOT USE
        /// @brief Get FB Color Texture
        /// @note Call after end_render_cache ONLY
        unsigned int get_texture();
    };
}
