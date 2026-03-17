#include "rocket/runtime.hpp"
#include "rocket/shader.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"

// Make sure to use rocket_main instead of main
// with extra rocket_arguments_t argument
int rocket_main(int argc, char **argv, rocket_arguments_t args) {
    rocket::init(argc, argv);
    rocket::window_t window({1280, 720}, "rgeExample - Custom Shader");
    rocket::renderer_2d r(&window, 60);

    rocket::shader_t shader(
        rocket::shader_type::vert_frag,
        args.working_dir + "resources/custom_shader.rlsl"
    );

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            r.draw_shader(shader);
            r.draw_fps();
        }
        r.end_frame();
        window.poll_events();
    }

    r.close();
    window.close();

    return 0;
}

// RocketGE will handle platform-specific main functions for you
// and make your game cross-platform!
DEFINE_PLATFORM_MAIN
