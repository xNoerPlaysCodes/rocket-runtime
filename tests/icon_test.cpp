#include "../include/rocket/runtime.hpp"
#include "rocket/asset.hpp"
#include "rocket/renderer.hpp"
#include "rocket/rgl.hpp"
#include "rocket/window.hpp"
#include "rocket/macros.hpp"

int main() {
#ifdef ROCKETGE__Platform_Linux
    // --------------------------------------
    // wayland doesn't support window icons!
    // --------------------------------------
    // wayland also needs a valid framebuffer
    // before showing the window
    // --------------------------------------
    rocket::window_t::set_forced_platform(rocket::platform_type_t::linux_x11);
#endif
    rocket::window_t window = { { 1280, 720 }, "RocketGE - Window Icon Test"};
    rocket::asset_manager_t asset_manager;
    rocket::assetid_t texture = asset_manager.load_texture("resources/window_icon.jpg", rocket::texture_color_format_t::rgba);
    window.set_icon(asset_manager.get_texture(texture));

    while (window.is_running()) {
        window.poll_events();
    }

    window.close();
    return 0;
}
