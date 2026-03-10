#include "../include/rocket/runtime.hpp"
#include "rocket/asset.hpp"
#include "rocket/io.hpp"
#include "rocket/renderer.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <iostream>

int main(int argc, char **argv) {
    rocket::register_argument("do-something-impressive", []() {
        std::cout << "okay but how\n";
    }, "Do something really impressive");
    rocket::register_argument("print-smth", [](std::string value) {
        std::cout << value << '\n';
    }, "Print Something", "str");
    rocket::init(argc, argv);
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::window_t window({1920, 1080}, "RocketGE - Argument Test");
    rocket::renderer_2d r(&window, 60, {.show_splash = !test_mode});

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
        }
        r.end_frame();
        window.poll_events();

        if (test_mode) return 0;
    }

    r.close();
    window.close();
}
