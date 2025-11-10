#include "../include/rocket/runtime.hpp"
#include "../include/astro/astroui.hpp"
#include "rocket/asset.hpp"
#include "rocket/renderer.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <iostream>
#include <memory>

rocket::rgba_color clr_gray() {
    return { 128, 128, 128, 255 };
}

rocket::rgba_color clr_lgray() {
    return { 192, 192, 192, 255 };
}

rocket::rgba_color clr_dgray() {
    return { 64, 64, 64, 255 };
}

rocket::rgba_color clr_blank() {
    return { 0, 0, 0, 0 };
}

void draw_info_util(astro::button_t &button, astro::draw_info_t &info) {
    if (button.is_clicking(true)) {
        info.color = rocket::rgba_color::black();
        info.text_color = rocket::rgb_color::white();
    } else if (button.is_hovering()) {
        info.color = clr_dgray();
        info.text_color = rocket::rgb_color::white();
    } else {
        info.color = clr_gray();
        info.text_color = rocket::rgb_color::black();
    }
}

int main(int argc, char **argv) {
    rocket::set_cli_arguments(argc, argv);
    rocket::windowflags_t flags;
    flags.resizable = true;
    flags.msaa_samples = 4;
    rocket::window_t window({ 1280, 720 }, "RocketGE - UI Init Test", flags);
    rocket::renderer_2d r(&window, 60, {
    });

    astro::set_renderer(&r);

    astro::button_t b1 = {"Yes", { 200 + 16, 200 + 400 - 16 - 75 }, { 150, 75 }};
    astro::button_t b2 = {"No", b1.pos + rocket::vec2f_t{ 200 + 20, 0 }, b1.size};
    rocket::text_t text = {"Do you agree?", 24, { 255, 255, 255 }};
    rocket::vec2f_t text_pos = {
        200.f + (400.f - text.measure().x) / 2.f,
        200.f + (400.f - text.font->get_line_height()) / 2.f - 16
    };
    astro::dialog_t dialog("Do you agree?", text_pos, { 200, 200 }, { 400, 400 }, &b1, &b2);

    while (window.is_running()) {
        r.begin_frame();
        r.clear(rocket::rgba_color::black());
        astro::begin_ui();
        {
            astro::draw_info_t info = {
                .color = clr_gray(),
                .text_color = rocket::rgb_color::black(),
                .border = {
                    .width = 0,
                    .radius = 0.25,
                    .color = clr_blank()
                }
            };
            astro::draw_info_t info_btn1 = info;
            astro::draw_info_t info_btn2 = info;
            info.border.radius = 0.05;
            draw_info_util(b1, info_btn1);
            draw_info_util(b2, info_btn2);
            info.color = rocket::rgba_color::white();
            dialog.draw(info, info_btn1, info_btn2);
        }
        {
            if (b1.is_clicking()) {
                rocket::log("Yes clicked", "main.cpp", "main", "info");
            }
            if (b2.is_clicking()) {
                rocket::log("No clicked", "main.cpp", "main", "info");
            }
        }
        astro::end_ui();
        r.draw_fps();
        r.end_frame();
        window.poll_events();
    }

    r.close();
    window.close();
}
