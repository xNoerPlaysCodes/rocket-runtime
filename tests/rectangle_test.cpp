#include "../include/runtime.hpp"
#include "../include/renderer.hpp"
#include "../include/io.hpp"
#include "../include/asset.hpp"
#include <cstdint>
#include <functional>
#include <iomanip>
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
        .fov = 90.f,
        .render_distance = 100.f,
        .pos = { 0, 0, 5 },
        .up = { 0, 1, 0 },
        .lookat = { 0, 0, 0 },
    };
    rocket::renderer_3d r(&w, &cam);

    w.set_cursor_mode(rocket::cursor_mode_t::hidden);

    bool dcam = true;

    float width = 1;
    float height = 1;
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

    rocket::asset_manager_t aman(1500s);

    std::shared_ptr<rocket::texture_t> textures[6] = {};

    for (auto &tx : textures) {
        tx = aman.get_texture(aman.load_texture("/home/noerlol/C-Projects/TopDownGame/assets/barrel.png"));
    }

    rocket::fbounding_box_3d fbox = { .pos = { 0, 0, 0 }, .size = { width, height, length } };
    rocket::fbounding_box_3d pbox = {
        .pos = cam.pos,
        .size = { 0.1, 0.1, 0.1 },
    };

    rocket::vec3f_t old_campos = cam.pos;
    while (w.is_running()) {
        r.begin_frame();
        r.clear();
        {
            // r.draw_texture(fbox, textures);
            r.draw_texture(fbox, textures);
        }
        {
            std::cout << "x:" << std::fixed << std::setprecision(10) << cam.pos.x << " y:" << cam.pos.y << " z:" << cam.pos.z << '\n';
        }
        r.end_frame();
        w.poll_events();
        {
            r.mouse_move(10);
        }
        {
            rocket::vec3f_t attempted_pos = cam.pos;

            std::cout << "[WARN] FIX COLLISION [WARN]\n";

            // // Check X
            // pbox.pos.x = attempted_pos.x;
            // if (pbox.intersects(fbox)) {
            //     pbox.pos.x = old_campos.x;
            //     cam.pos.x = old_campos.x;
            // }
            //
            // // Check Y
            // pbox.pos.y = attempted_pos.y;
            // if (pbox.intersects(fbox)) {
            //     pbox.pos.y = old_campos.y;
            //     cam.pos.y = old_campos.y;
            // }
            //
            // // Check Z
            // pbox.pos.z = attempted_pos.z;
            // if (pbox.intersects(fbox)) {
            //     pbox.pos.z = old_campos.z;
            //     cam.pos.z = old_campos.z;
            // }
        }
        {
            pbox.pos = cam.pos;
            old_campos = cam.pos;
        }
        fbox.size = { width, height, length };
    }

    r.close();
    w.close();
    return 0;
}
