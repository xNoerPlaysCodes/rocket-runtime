#include "rocket/renderer.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <rocket/runtime.hpp>

int main(int argc, char **argv) {
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::window_t window({ 1280, 720 }, "RocketGE - Line Test");
    rocket::renderer_2d r(&window, 60, {.show_splash = !test_mode});

    // rocket::log("Test Failed: draw_line() is not implemented", "main.cpp", "main", "fatal");
    // rocket::exit(-1);

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            float x = r.get_viewport_size().x; float y = r.get_viewport_size().y;
            r.draw_line({ 50, 50 }, { 200, 200 }, rocket::rgba_color::red(), 1.f);
            r.draw_rectangle({
                .pos = { 100, 300 },
                .size = { 100, 100 }
            }, rocket::rgba_color::red(), 0, 0, true);

            r.draw_fps();
        }
        r.end_frame();
        window.poll_events();
        if (test_mode) return 0;
    }
}
