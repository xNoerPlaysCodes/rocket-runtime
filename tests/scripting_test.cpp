#include "rocket/renderer.hpp"
#include "rocket/window.hpp"
#include <chrono>
#include <rocket/runtime.hpp>
#include <rocket/scripting.hpp> 
#include <thread>

int main(int argc, char **argv) {
    rocket::init(argc, argv);
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::window_t window = { { 1280, 720 }, "RocketGE - Scripting Test" };
    rocket::renderer_2d r = { &window, 60, { .show_splash = !test_mode } };

    rocket::script::init();
    rocket::script::environment_t env;
    std::string code = R"(
import rocket

r2d = rocket.get_renderer2d()
color = rocket.rgba_color()
color.x = 14
color.y = 14
color.z = 14

r2d.clear(color)
pos = rocket.vec2f(100, 100)
size = rocket.vec2f(200, 200)
r2d.draw_rectangle(pos, size)
rocket.log("Hello, world!")
)";

    r.begin_frame();
    r.clear();
    bool success = env.exec(code);
    r.end_frame();

    if (success)
        std::this_thread::sleep_for(std::chrono::seconds(2));

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
        }
        r.end_frame();
        window.poll_events();

        if (test_mode) return success;
    }
}
