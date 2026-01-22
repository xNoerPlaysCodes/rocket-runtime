#include "../include/rocket/runtime.hpp"
#include "rocket/asset.hpp"
#include "rocket/io.hpp"
#include "rocket/renderer.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"

int main(int argc, char **argv) {
    rocket::set_cli_arguments(argc, argv);
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::window_t window({1920, 1080}, "RocketGE - Text Test");
    rocket::renderer_2d r(&window, 60, {.show_splash = !test_mode});

    rocket::text_t text = {"Hello, Rocket World!", 48, rocket::rgb_color::black()};

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            r.draw_text(text, { static_cast<float>(window.get_size().x) / 2 - text.measure().x / 2, static_cast<float>(window.get_size().y) / 2 - text.measure().y / 2 });
        }
        {
            r.draw_fps();
        }
        r.end_frame();
        window.poll_events();

        if (test_mode) return 0;
    }

    r.close();
    window.close();
}
