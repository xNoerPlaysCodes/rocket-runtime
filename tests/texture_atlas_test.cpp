#include "rocket/asset.hpp"
#include "rocket/renderer.hpp"
#include "rocket/rgl.hpp"
#include <chrono>
#include <rocket/runtime.hpp>
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <rocket/threads.hpp>
#include <sstream>
#include <string>
#include <thread>

int main(int argc, char **argv) {
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::init(argc, argv);
    rocket::window_t window({ 1280, 720 }, "RocketGE - Multithreaded Test", {
    });
    rocket::renderer_2d r(&window, 60, {
        .show_splash = !test_mode
    });

    rocket::asset_manager_t am;
    std::shared_ptr<rocket::texture_t> tx = nullptr;

    tx = am.get_texture(am.load_texture("resources/atlas.png"));

    r.begin_render_mode(rocket::render_mode_t::texture_filter_none);

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {       
            r.draw_rectangle({ 100, 100 }, { 256, 256 }, rocket::rgba_color::red());
            r.draw_atlas_texture(tx, { {250, 250}, {256,256} }, { 0, 0 }, { 16, 16 });
            r.draw_fps();
        }
        r.end_frame();
        window.poll_events();
        if (test_mode) {
            if (tx != nullptr) return 0;
        }
    }

    window.close();
}

