#include "rocket/persistence.hpp"
#include "rocket/renderer.hpp"
#include "rocket/window.hpp"
#include <iostream>
#include <rocket/runtime.hpp>

int variant_to_int(const rocket::storage::variable_t &v, int default_value) {
    return std::visit([default_value](auto&& x) -> int {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_arithmetic_v<T>) return static_cast<int>(x);
        else return default_value;
    }, v);
}

int main(int argc, char **argv) {
    rocket::init(argc, argv);
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }

    rocket::storage::init("persistence_test");

    rocket::storage::variable_t width = rocket::storage::get("window_width");
    rocket::storage::variable_t height = rocket::storage::get("window_height");

    int w = variant_to_int(width, 1280);
    int h = variant_to_int(height, 720);

    rocket::window_t window = { { w, h }, "RocketGE - Persistence Test" };
    rocket::renderer_2d r = { &window, 60, { .show_splash = !test_mode } };

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            
        }
        r.end_frame();
        window.poll_events();

        if (test_mode) return 0;
    }

    rocket::storage::store("window_width", window.get_size().x);
    rocket::storage::store("window_height", window.get_size().y);
    rocket::storage::flush();
}
