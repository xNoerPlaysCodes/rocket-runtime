#include "../include/rocket/runtime.hpp"
#include "../include/rocket/asset.hpp"
#include "../include/rocket/io.hpp"
#include "../include/rocket/renderer.hpp"
#include "../include/rocket/types.hpp"
#include "../include/rocket/window.hpp"
#include <algorithm>
#include <iostream>

int main() {
    rocket::windowflags_t window_flags = {
        .msaa_samples = 4
    };
    rocket::window_t window({ 1920, 1080 }, "RocketGE - ExampleWindow", window_flags);
    rocket::renderer_2d r2d(&window);

    rocket::asset_manager_t asset_manager;
    rocket::assetid_t txid = asset_manager.load_texture("/home/noerlol/C-Projects/TopDownGame/assets/barrel.png");

    float deg = 0;

    rocket::io::add_listener([&](rocket::io::key_event_t e) {
        if (!e.state.down()) {
            return;
        }
        if (e.key == rocket::io::keyboard_key::w) {
            deg += 1.f;
        }
        if (e.key == rocket::io::keyboard_key::s) {
            deg -= 1.f;
        }
    });

    while (window.is_running()) {
        r2d.begin_frame();
        r2d.clear();
        {
            std::cout << deg << '\n';
            deg = std::clamp(deg, 0.f, 360.f);
            r2d.draw_texture(asset_manager.get_texture(txid), {
                .pos = {
                    .x = window.get_size().x / 2 - 360,
                    .y = window.get_size().y / 2 - 360,
                },
                .size = {
                    .x = 720,
                    .y = 720
                }
            }, deg);
        }
        r2d.end_frame();
        window.poll_events();
    }

    window.close();
    r2d.close();
}
