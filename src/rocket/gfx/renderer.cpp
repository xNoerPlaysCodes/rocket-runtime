#include "rocket/renderer.hpp"
#include <rocket/renderer_helpers.hpp>
#include <util.hpp>
#include <stack>

#define MAJOR(x) ((x) / 10)
#define MINOR(x) ((x) % 10)

#define VERSION(mj, mn) (((mj) * 10) + (mn))

namespace rocket {
    namespace {
        struct device_capability_t {
            int max_gl_version = 0;
            int max_vk_version = 0;
        };

        std::unique_ptr<renderer_2d_i> get_renderer(renderer_backend_t backend, window_backend_i *window, int fps, const renderer_flags_t &flags) {
            switch (backend) {
                case renderer_backend_t::opengl:
                    return std::make_unique<opengl_renderer_2d>(window, fps, flags);
                case rocket::renderer_backend_t::vulkan:
                    return std::make_unique<vulkan_renderer_2d>(window, fps, flags);
                case rocket::renderer_backend_t::null:
                    return std::make_unique<null_renderer_2d>();
                default: return std::make_unique<null_renderer_2d>();
            }
        }

        int test_backend(const renderer_backend_t &backend, rocket::window_backend_i *win) {
            constexpr auto test_gl = [](rocket::window_backend_i *win) -> int {
                auto ver = win->get_flags().graphics_ctx.version;
                return VERSION(ver.x, ver.y);
            };
            constexpr auto test_vk = [](rocket::window_backend_i *) -> int {
                return 0;
            };
            switch (backend) {
                case renderer_backend_t::opengl:
                    return test_gl(win);
                case rocket::renderer_backend_t::vulkan:
                    return test_vk(win);
                case rocket::renderer_backend_t::null:
                    return VERSION(1, 0);
                default: return 0;
            }
        }

        device_capability_t get_capabilities(rocket::window_backend_i *win) {
            device_capability_t cap;
            cap.max_gl_version = test_backend(renderer_backend_t::opengl, win);
            cap.max_vk_version = VERSION(6, 7);
            return cap;
        }

        std::stack<renderer_backend_t> get_preferred_backend_list(rocket::renderer_backend_t requested, rocket::window_backend_i *win, uint8_t choice) {
            device_capability_t caps = get_capabilities(win);
            std::stack<renderer_backend_t> stk;
            stk.push(renderer_backend_t::null);
            if (caps.max_vk_version != 0 && choice & renderer_choice::vulkan)
                stk.push(renderer_backend_t::vulkan);
            if (caps.max_gl_version != 0 && choice & renderer_choice::opengl)
                stk.push(renderer_backend_t::opengl);
            stk.push(requested);
            auto cli_args = util::get_clistate();
            if (cli_args.renderer_backend_version > 0)
                stk.push(cli_args.renderer_backend);

            return stk;
        }
    }

    std::unique_ptr<renderer_2d_i> create_renderer_2d(
        renderer_backend_t backend, 
        window_backend_i *window, 
        int fps, 
        renderer_flags_t flags, 
        uint8_t renderer_choice_bitmask
    ) {
        return get_renderer(
            get_preferred_backend_list(
                backend, 
                window, 
                renderer_choice_bitmask
            ).top(), 
            window, fps, flags
        );
    }
}
