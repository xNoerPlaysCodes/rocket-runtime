#include "astro/astroui.hpp"
#include "editor/page.hpp"
#include "rocket/asset.hpp"
#include "rocket/rgl.hpp"
#include "rocket/shader.hpp"
#include "rocket/types.hpp"
#include <rocket/renderer.hpp>
#include <rocket/runtime.hpp>
#include <rocket/window.hpp>

constexpr int pg_main_menu = 0;
constexpr int pg_create_project = 1;
constexpr int pg_open_project = 2;
constexpr int pg_editor = 3;

int main(int argc, char **argv) {
    rocket::set_log_level(rocket::log_level_t::warn);
    rocket::init(argc, argv);
    rocket::window_t window = { { 1600, 900 }, "RocketGE - Editor", {
        .vsync = true,
        .msaa_samples = 8,
    } };

    rocket::renderer_2d r = { &window, 60, {
        .show_splash = false
    } };

    astro::set_renderer(&r);

    rocket::asset_manager_t am;
    rocket::assetid_t bg = am.load_texture("bin/resources/wallpaper.png");

    int page = 0;

    auto bcallback = [&](const astro::button_t &b, std::string page_str) {
        if (page_str == "main_menu" && b.text == "New") {
            page = pg_create_project;
        } else {
            rocket::log("Unknown Page!", "main.cpp", "main", "error");
        }
    };

    while (window.is_running()) {
        r.begin_frame();
        r.clear(rocket::rgba_color::black());
        astro::begin_ui();
        {
            if (page == pg_main_menu)
                rgeditor::draw_page_main_menu(window, r, am, bcallback, bg);
            else if (page == pg_create_project)
                rgeditor::draw_page_create_project(window, r, am, bcallback, bg);
            else
                rocket::log("Unknown Page!", "main.cpp", "main", "error");
        }
        astro::end_ui();
        r.end_frame();
        window.poll_events();
    }
}
