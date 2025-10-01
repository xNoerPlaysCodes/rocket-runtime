#include "rocket/asset.hpp"
#include "rocket/io.hpp"
#include "rocket/renderer.hpp"
#include <rocket/window.hpp>
#include <string>

std::string bool_to_str(bool b) {
    return b ? "true" : "false";
}

int main() {
    rocket::window_t window = { { 1280, 720 }, "RocketGE - IO Test" };
    rocket::renderer_2d r(&window);

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            std::vector<std::string> strs = {};
            strs.reserve(8);
            if (rocket::io::mouse_down(rocket::io::mouse_button::left)) {
                strs.push_back("LMB");
            } if (rocket::io::mouse_down(rocket::io::mouse_button::right)) {
                strs.push_back("RMB");
            } if (rocket::io::mouse_down(rocket::io::mouse_button::middle)) {
                strs.push_back("MMB");
            } if (rocket::io::mouse_down(rocket::io::mouse_button::button_4)) {
                strs.push_back("B4");
            } if (rocket::io::mouse_down(rocket::io::mouse_button::button_5)) {
                strs.push_back("B5");
            } if (rocket::io::mouse_down(rocket::io::mouse_button::button_6)) {
                strs.push_back("B6");
            } if (rocket::io::mouse_down(rocket::io::mouse_button::button_7)) {
                strs.push_back("B7");
            } if (rocket::io::mouse_down(rocket::io::mouse_button::button_8)) {
                strs.push_back("B8");
            }

            rocket::text_t t = { "", 32, rocket::rgb_color::black(), rGE__FONT_DEFAULT_MONOSPACED };
            for (auto &s : strs) {
                t.text += s + " ";
            }
            if (t.text.empty()) {
                t.text = "Press any mouse key to test";
            }
            r.draw_text(t, r.get_viewport_size() / 2.f - t.measure() / 2.f);
            r.draw_fps();
        }
        r.end_frame();
        window.poll_events();
    }
}
