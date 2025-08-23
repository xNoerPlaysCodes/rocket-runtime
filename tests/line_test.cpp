#include "rocket/renderer.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <rocket/runtime.hpp>

int main() {
    rocket::window_t window({ 1280, 720 }, "RocketGE - Line Test");
    rocket::renderer_2d r(&window);

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            r.draw_line({ 100, 100 }, { 200, 200 }, rocket::rgba_color::red(), 4.f);
        }
        r.end_frame();
        window.poll_events();
    }
}
