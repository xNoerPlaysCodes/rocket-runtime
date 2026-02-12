#include "rocket/renderer.hpp"
#include "rocket/runtime.hpp"
#include "rocket/plugin/plugin.hpp"
#include "rocket/window.hpp"
#include <iostream>

int main(int argc, char **argv) {
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::init(argc, argv);    
    rocket::set_log_level(rocket::log_level_t::warn);
    auto plugin = rocket::load_plugin("resources/test.plugin");
    if (plugin != nullptr) {
        void (*my_test)() = reinterpret_cast<void(*)()>(plugin->get_function("my_test"));

        my_test();
    } else {
        rocket::log("you didn't let it load the plugin!", "main.cpp", "main", "warn");
    }

    rocket::window_t window = { { 1280, 720 }, "RocketGE - Plugin Test" };
    rocket::renderer_2d r(&window, 60, {.show_splash = !test_mode});

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            // plugin renders first
            r.draw_rectangle({
                .pos = { 100, 300 },
                .size = { 100, 100 }
            }, rocket::rgba_color::red());
            // or use a plugin-specific drawing callback
        }
        r.end_frame();
        window.poll_events();

        if (test_mode) return plugin == nullptr;
    }
}
