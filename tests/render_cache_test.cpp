#include "rocket/asset.hpp"
#include "rocket/io.hpp"
#include "rocket/renderer.hpp"
#include "rocket/types.hpp"
#include <rocket/window.hpp>
#include <rocket/runtime.hpp>
#include <string>

int rocket_main(int argc, char **argv, rocket_arguments_t) {
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::init(argc, argv);
    rocket::window_t window = { { 1280, 720 }, "RocketGE - Render Cache Test" };
    rocket::renderer_2d r(&window, 60, {.show_splash = !test_mode});

    r.begin_frame();
    auto cache = r.create_render_cache([](rocket::renderer_2d *ren) {
        ren->clear(rocket::rgba_color(200,200,200));
        ren->draw_rectangle({{300, 20}, { 240, 240 }}, rocket::rgba_color::red());
        rocket::text_t text = {"Hello, world!", 32, rocket::rgb_color::black()};
        ren->draw_text(text, { 200, 450 });
    });

    r.end_frame();



    while (window.is_running()) {
        r.begin_frame();
        r.clear(rocket::rgba_color::white());
        {
            r.draw_render_cache(cache, {{0,0},r.get_viewport_size()});
        }
        r.draw_fps();
        r.end_frame();
        window.poll_events();
        if (test_mode) return 0;
    }

    return 0;
}

DEFINE_PLATFORM_MAIN
