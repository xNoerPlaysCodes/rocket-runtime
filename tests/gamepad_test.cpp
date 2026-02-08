#include "rocket/asset.hpp"
#include "rocket/renderer.hpp"
#include "rocket/rgl.hpp"
#include "rocket/runtime.hpp"
#include "rocket/io.hpp"
#include "rocket/window.hpp"
#include <sstream>
#include <string>

int main(int argc, char **argv) {
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::window_t::set_forced_platform(rocket::platform_type_t::linux_x11);
    rocket::window_t window = { { 1280, 720 }, "RocketGE - Gamepads" };
    rocket::renderer_2d r(&window, 60, {.show_splash = !test_mode});

    rocket::text_t text = { "", 48, rocket::rgb_color::black() };

    int hd = 0;
    if (auto gpads = rocket::gpad::get_available_gamepads(); gpads.size() > 0) {
        hd = gpads.at(0);
    }

    if (!rocket::gpad::is_available(hd)) {
        rocket::log("Gamepad is not available", "main.cpp", "main", "fatal");
        return 1;
    }
    rocket::gpad::gamepad_t gamepad = rocket::gpad::get_handle(hd);

    rocket::io::add_listener([](rocket::io::mouse_event_t event) {
        if (!event.state.pressed()) {
            return;
        }

        std::string logstr = "";
        if (event.button == rocket::io::mouse_button::left) {
            logstr = "LMB";
        } else if (event.button == rocket::io::mouse_button::right) {
            logstr = "RMB";
        } else if (event.button == rocket::io::mouse_button::middle) {
            logstr = "MMB";
        } else {
            logstr = "???";
        }
        rocket::log(logstr, "main.cpp", "mouse_listener", "info");
    });

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            std::stringstream ss;
            ss << "L2: " + std::to_string(rocket::gpad::get_axis_state(gamepad, rocket::gpad::axis_t::l2)) << " && ";
            ss << "R2: " + std::to_string(rocket::gpad::get_axis_state(gamepad, rocket::gpad::axis_t::r2));

            if (rocket::gpad::get_button_state(gamepad, rocket::gpad::button_t::a)) {
                ss << " && A";
                rocket::io::simulate(rocket::io::mouse_button::left, rocket::io::keystate_t::make_pressed(), rocket::io::mouse_pos());
            }

            text.text = ss.str();

            rocket::vec2f_t screen_size = r.get_viewport_size();
            rocket::vec2f_t text_size = text.measure();

            rocket::vec2f_t position = (screen_size / 2.0f) - (text_size / 2.0f);

            r.draw_text(text, position);

            r.draw_fps();

            rocket::gpad::set_vibration(gamepad, 1.0f, 1000);
        }
        r.end_frame();
        window.poll_events();

        if (test_mode) {
            return 0;
        }
    }
}
