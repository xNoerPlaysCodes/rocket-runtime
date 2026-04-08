#include "rocket/runtime.hpp"
#include "rocket/shader.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <memory>

// Make sure to use rocket_main instead of main
// with extra rocket_arguments_t argument
int rocket_main(int argc, char **argv, rocket_arguments_t args) {
    rocket::init(argc, argv);
    rocket::window_t window({1280, 720}, "rgeExample - Custom Shader");
    auto r = rocket::create_renderer_2d(window.get_flags().graphics_ctx.backend, &window, 60);

    std::unique_ptr<rocket::shader_i> shader;
    if (window.get_flags().graphics_ctx.backend == rocket::renderer_backend_t::vulkan) {
        shader = std::make_unique<rocket::vulkan_shader_t>(
            rocket::shader_type::vert_frag,
            std::filesystem::path(args.working_dir + "resources/custom_shader.rlsl")
        );
    } else {
        shader = std::make_unique<rocket::opengl_shader_t>(
            rocket::shader_type::vert_frag,
            std::filesystem::path(args.working_dir + "resources/custom_shader.rlsl")
        );
    }

    while (window.is_running()) {
        r->begin_frame();
        r->clear();
        {
            r->draw_shader(*shader);
            r->draw_fps();
        }
        r->end_frame();
        window.poll_events();
    }

    r->close();
    window.close();

    return 0;
}

// RocketGE will handle platform-specific main functions for you
// and make your game cross-platform!
DEFINE_PLATFORM_MAIN
