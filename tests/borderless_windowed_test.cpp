#include "rocket/renderer.hpp"
#include "rocket/window.hpp"
#include <iostream>
#include <rocket/runtime.hpp>
#include <string>

int main(int argc, char **argv) {
    rocket::init(argc, argv);
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }

    rocket::window_t::cpl_init();
    rocket::monitor_t main_mon = rocket::monitor_t::of(0);
    rocket::window_t window = { main_mon.size, "RocketGE - Borderless Windowed" };
    rocket::renderer_2d r = { &window, 60, { .show_splash = !test_mode } };

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        r.end_frame();
        window.poll_events();

        if (test_mode) return window.get_size() != main_mon.size;
    }
}
