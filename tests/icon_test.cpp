#include "../include/rocket/runtime.hpp"
#include "rocket/asset.hpp"
#include "rocket/renderer.hpp"
#include "rocket/rgl.hpp"
#include "rocket/window.hpp"
#include "rocket/macros.hpp"

int main(int argc, char **argv) {
#ifdef ROCKETGE__Platform_Linux
    // --------------------------------------
    // wayland doesn't support window icons!
    // --------------------------------------
    // wayland also needs a valid framebuffer
    // before showing the window
    // --------------------------------------
    rocket::window_t::set_forced_platform(rocket::platform_type_t::linux_x11);
#endif
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::init(argc, argv);
    rocket::window_t window = { { 1280, 720 }, "RocketGE - Window Icon Test"};
    rocket::renderer_2d r = { &window, 60, { .show_splash = !test_mode } };
    rocket::asset_manager_t asset_manager;
    rocket::assetid_t texture = asset_manager.load_texture("resources/window_icon.jpg", rocket::texture_color_format_t::rgba);
    window.set_icon(asset_manager.get_texture(texture));

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        r.end_frame();
        window.poll_events();
        if (test_mode) {
            return 0;
        }
    }

    window.close();
    return 0;
}
