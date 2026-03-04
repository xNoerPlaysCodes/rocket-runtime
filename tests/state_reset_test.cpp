#include "../include/rocket/runtime.hpp"
#include "rocket/asset.hpp"
#include "rocket/io.hpp"
#include "rocket/renderer.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"

int main(int argc, char **argv) {
    rocket::init(argc, argv);
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::window_t window({1920, 1080}, "RocketGE - State Reset Test");
    rocket::renderer_2d r(&window, 60, {.show_splash = !test_mode});

    rocket::text_t text = {"Hello, Rocket World!", 48, rocket::rgb_color::black()};

    int i = 0;
    while (window.is_running() && i < 20) {
        r.begin_frame();
        r.clear();
        {
        }
        {
        }
        r.end_frame();
        window.poll_events();
        ++i;
    }

    r.close();
    window.close();

    {
        rocket::window_t w2 = {};
        rocket::renderer_2d r2 = &w2;
        while (w2.is_running()) {
            r2.begin_frame();
            r2.clear();
            {
                r2.draw_rectangle({ {0,0}, {240,240} }, rocket::rgba_color::red());
            }
            r2.end_frame();
            w2.poll_events();
        }
    }
}
