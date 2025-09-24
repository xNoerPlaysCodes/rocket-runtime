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
            float x = r.get_viewport_size().x; float y = r.get_viewport_size().y;
            r.draw_line({ 0, 0 }, { x, y }, rocket::rgba_color::red(), 1.f);
            r.draw_rectangle({
                .pos = { 100, 300 },
                .size = { 100, 100 }
            }, rocket::rgba_color::red(), 0, 0, true);

            r.draw_fps();
        }
        r.end_frame();
        window.poll_events();
    }
}
