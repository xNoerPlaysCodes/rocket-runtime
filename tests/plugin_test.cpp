#include "rocket/renderer.hpp"
#include "rocket/runtime.hpp"
#include "rocket/plugin/plugin.hpp"
#include "rocket/window.hpp"

int main() {
    rocket::set_log_level(rocket::log_level_t::warn);
    auto plugin = rocket::load_plugin("resources/test.plugin");
    void (*my_test)() = reinterpret_cast<void(*)()>(plugin->get_function("my_test"));

    my_test();

    rocket::window_t window = { { 1280, 720 }, "RocketGE - Plugin Test" };
    rocket::renderer_2d r(&window);

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
    }
}
