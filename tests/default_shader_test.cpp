#include "rocket/renderer.hpp"
#include "rocket/window.hpp"
#include <rocket/runtime.hpp>

int main(int argc, char **argv) {
    rocket::init(argc, argv);
    rocket::window_t window = { { 1280, 720 }, "RocketGE - Default Shader Test" };
    rocket::renderer_2d r = { &window };

    int th = 0;
    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            r.draw_rectangle({ {20, 20}, {200, 200} });
            r.draw_circle({ 120, 350 }, 100, { 0, 0, 0, 255 }, 4);
        }
        r.draw_fps();
        r.end_frame();
        window.poll_events();
    }
}
