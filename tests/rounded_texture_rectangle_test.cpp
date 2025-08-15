#include "../include/rocket/runtime.hpp"
#include "rocket/asset.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"

int main() {
    rocket::window_t window({ 1280, 720 }, "RocketGE - Rounded Texture Rectangle Test");
    rocket::renderer_2d r(&window);

    rocket::asset_manager_t am;
    rocket::assetid_t txid = am.load_texture("/home/noerlol/C-Projects/TopDownGame/assets/barrel.png");

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            r.draw_texture(am.get_texture(txid), { { 100, 100 }, { 128, 128 } }, 0, 0.3f);
        }
        r.end_frame();
        window.poll_events();
    }
}
