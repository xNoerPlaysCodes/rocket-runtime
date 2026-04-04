#include "rocket/renderer.hpp"

namespace rocket {
    std::unique_ptr<renderer_2d_i> create_renderer_2d(renderer_backend_t backend, window_backend_i *window, int fps, renderer_flags_t flags) {
        switch (backend) {
            case renderer_backend_t::opengl:
                return std::make_unique<opengl_renderer_2d>(window, fps, flags);
            case rocket::renderer_backend_t::vulkan: // TODO Vulkan
                return std::make_unique<vulkan_renderer_2d>(window, fps, flags);
            case rocket::renderer_backend_t::null:
                return std::make_unique<null_renderer_2d>();
            default: return std::make_unique<null_renderer_2d>();
        }
    }
}
