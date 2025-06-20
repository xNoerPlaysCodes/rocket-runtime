#include "../include/runtime.hpp"
#include "../include/renderer.hpp"
#include "../include/io.hpp"
#include "../include/asset.hpp"
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

using namespace std::chrono_literals;

int main() {
    rocket::windowflags_t flags = {
        .msaa_samples = 4,
    };
    rocket::window_t w({ 1920, 1080 }, "RocketGE - Test", flags);
    rocket::camera3d_t cam = {
        .fov = 45.f,
        .render_distance = 100.f,
        .pos = { 0, 0, 5 },
        .up = { 0, 1, 0 },
        .lookat = { 0, 0, 0 },
    };
    rocket::renderer_3d r(&w, &cam);

    w.set_cursor_mode(rocket::cursor_mode_t::hidden);

    bool dcam = true;

    float width = 2;
    float height = 3;
    float length = 1;

    float *ref = &width;
    rocket::io::add_listener([&](rocket::io::key_event_t e) {
        if (!e.state.down()) {
            return;
        }

        float spd = 2.f;
        if (e.key == rocket::io::keyboard_key::w) {
            r.move_cam_forward(1.f * spd, true, false, false);
        } else if (e.key == rocket::io::keyboard_key::s) {
            r.move_cam_backward(1.f * spd, true, false, false);
        } else if (e.key == rocket::io::keyboard_key::a) {
            r.move_cam_left(1.f * spd);
        } else if (e.key == rocket::io::keyboard_key::d) {
            r.move_cam_right(1.f * spd);
        }
        if (e.key == rocket::io::keyboard_key::equal) {
            *ref += 0.1f;
        }
        if (e.key == rocket::io::keyboard_key::minus) {
            *ref -= 0.1f;
        }
    });

    rocket::io::add_listener([&](rocket::io::key_event_t e) {
        if (!e.state.pressed()) {
            return;
        }

        if (e.key == rocket::io::keyboard_key::space) {
            if (*ref == width) {
                ref = &height;
            } else if (*ref == height) {
                ref = &length;
            } else if (*ref == length) {
                ref = &width;
            }
        }
    });

    while (w.is_running()) {
        r.begin_frame();
        r.clear();
        {
            r.draw_rectangle({ 0, 0, 0 }, { width, height, length }, { 255, 0, 0, 255 });
            r.draw_camera();
        }
        {
            std::cout << "x:" << cam.pos.x << " y:" << cam.pos.y << " z:" << cam.pos.z << '\n';
        }
        r.end_frame();
        w.poll_events();
        {
            r.mouse_move(10);
        }
    }

    r.close();
    w.close();
    return 0;
}
