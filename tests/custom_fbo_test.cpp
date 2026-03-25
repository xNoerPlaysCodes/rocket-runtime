#include "rocket/asset.hpp"
#include "rocket/audio.hpp"
#include "rocket/io.hpp"
#include "rocket/renderer.hpp"
#include "rocket/rgl.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <rocket/runtime.hpp>
#include <astro/astroui.hpp>

int rocket_main(int argc, char **argv, rocket_arguments_t) {
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }

    rocket::init(argc, argv);
    rocket::window_t window({1280, 720}, "RocketGE - Custom FBO Test", {
        .msaa_samples = 4,
    });
    rocket::renderer_2d ren(&window, 60, {
        .show_splash = !test_mode
    });
    rocket::asset_manager_t am;

    rgl::fbo_t fbo = rgl::create_fbo();
    rgl::use_fbo(fbo);
    rgl::gl_viewport({0,0}, ren.get_viewport_size());
    rgl::gl_clear(rgl::gl_color_buffer_bit | rgl::gl_depth_buffer_bit, { 0, 255, 0, 255 });
    rgl::reset_to_default_fbo();

    while (window.is_running()) {
        static rocket::vec2f_t last_vp_size = ren.get_viewport_size();
        window.poll_events();
        ren.clear();
        ren.begin_frame();
        rgl::use_fbo(fbo);
        rgl::gl_viewport({0,0}, ren.get_viewport_size());
        ren.clear();
        {
            ren.draw_rectangle({ { 200, 200 }, { 240, 240 } }, rocket::rgba_color::red());
        }
        rgl::reset_to_default_fbo();

        ren.draw_fps();
        ren.end_frame();
        if (last_vp_size != ren.get_viewport_size()) {
            rgl::delete_fbo(fbo);
            fbo = rgl::create_fbo();
            rgl::use_fbo(fbo);
            rgl::gl_viewport({0,0}, ren.get_viewport_size());
            rgl::gl_clear(rgl::gl_color_buffer_bit | rgl::gl_depth_buffer_bit, { 0, 255, 0, 255 });
            rgl::reset_to_default_fbo();
        }
        last_vp_size = ren.get_viewport_size();

        if (test_mode) return fbo != rGL_FBO_INVALID ? 0 : 1;
    }

    return 0;
}

DEFINE_PLATFORM_MAIN
