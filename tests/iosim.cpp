#include "rocket/asset.hpp"
#include "rocket/io.hpp"
#include "rocket/renderer.hpp"
#include "rocket/runtime.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <cstring>
#include <string>

int main(int argc, char **argv) {
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::window_t window = { {1280, 720}, "RocketGE - IO Simulation" };
    rocket::renderer_2d r(&window, 60, {.show_splash = !test_mode});
    rocket::text_t instructions = {
        "Press the Arrow Keys for WASD",
        24, rocket::rgb_color::black()
    };

    rocket::io::add_listener([](rocket::io::key_event_t e) {
        if (!e.state.down()) {
            return;
        }
    });

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        std::string tx = "";

        auto stradd = [](std::string &a, std::string b) {
            if (a.ends_with(" &&")) {
                a += b + " &&";
            }
        };

        auto strstrip = [](std::string &a) {
            if (a.ends_with(" &&")) {
                a = a.substr(0, a.length() - 3);
            }
        };

        if (rocket::io::key_down(rocket::io::keyboard_key::up)) {
            rocket::io::simulate(rocket::io::keyboard_key::w, rocket::io::keystate_t::make_down());
        }

        if (rocket::io::key_down(rocket::io::keyboard_key::down)) {
            rocket::io::simulate(rocket::io::keyboard_key::s, rocket::io::keystate_t::make_down());
        }

        if (rocket::io::key_down(rocket::io::keyboard_key::left)) {
            rocket::io::simulate(rocket::io::keyboard_key::a, rocket::io::keystate_t::make_down());
        }

        if (rocket::io::key_down(rocket::io::keyboard_key::right)) {
            rocket::io::simulate(rocket::io::keyboard_key::d, rocket::io::keystate_t::make_down());
        }

        if (rocket::io::key_down(rocket::io::keyboard_key::w)) {
            stradd(tx, "W");
        }

        if (rocket::io::key_down(rocket::io::keyboard_key::s)) {
            stradd(tx, "S");
        }

        if (rocket::io::key_down(rocket::io::keyboard_key::a)) {
            stradd(tx, "A");
        }

        if (rocket::io::key_down(rocket::io::keyboard_key::d)) {
            stradd(tx, "D");
        }

        r.draw_text(instructions, { 0, 0 });
        strstrip(tx);
        rocket::text_t text = { tx, 24, rocket::rgb_color::black() };
        r.draw_text(text, { 0, 24 });

        r.end_frame();
        window.poll_events();
        if (test_mode) return 0;
    }
}
