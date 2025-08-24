#include "rocket/io.hpp"
#include "rocket/renderer.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <rocket/runtime.hpp>

int main() {
    rocket::window_t window({1280, 720}, "rgeExample - Moving Circle");
    rocket::renderer_2d r(&window, 60);

    rocket::vec2f_t position = { 100, 100 };

    rocket::io::add_listener([&position, &window](rocket::io::key_event_t event) {
        if (!event.state.down()) return;

        if (event.key == rocket::io::keyboard_key::w) {
            position.y -= 10;
        } else if (event.key == rocket::io::keyboard_key::s) {
            position.y += 10;
        } else if (event.key == rocket::io::keyboard_key::a) {
            position.x -= 10;
        } else if (event.key == rocket::io::keyboard_key::d) {
            position.x += 10;
        }

        position.x = std::clamp(position.x, 0.f, (float) window.get_size().x);
        position.y = std::clamp(position.y, 0.f, (float) window.get_size().y);
    });

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            r.draw_circle(position, 50, { 255, 0, 0, 255 });
            r.draw_fps();
        }
        r.end_frame();
        window.poll_events();
    }

    r.close();
    window.close();
}
