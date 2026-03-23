#include "rocket/asset.hpp"
#include "rocket/renderer.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <cstdlib>
#include <rocket/runtime.hpp>

int rocket_main(int argc, char **argv, rocket_arguments_t args) {
    rocket::init(argc, argv);
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::window_t window = { { 1280, 720 }, "RocketGE - Default Shader Test" };
    rocket::renderer_2d r = { &window, 60, {
        .show_splash = !test_mode
    } };

    rocket::asset_manager_t am;
    auto atlas_tx = am.load_texture(args.working_dir + "resources/atlas.png");
    auto arrow_tx = am.load_texture(args.working_dir + "resources/arrow_right.png");

    r.begin_render_mode(rocket::render_mode_t::texture_filter_none);
    r.make_ready_texture(am.get_texture(atlas_tx));
    r.end_render_mode(rocket::render_mode_t::texture_filter_none);

    std::srand(std::time(nullptr));

    float rotation = 0.f;
    rocket::rgba_color color = rocket::rgba_color::black();
    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            r.draw_rectangle({ {20, 20}, {200, 200} });
            r.draw_circle({ 120, 350 }, 100, { 0, 0, 0, 255 }, 4);
            r.draw_circle({ 120, 350 }, 85, { 0, 0, 0, 255 });
            r.draw_pixel({ 120 + 90, 350 }, rocket::rgba_color::red());
            r.draw_text({"The quick brown fox jumps over the lazy dog", 32, rocket::rgb_color::black()}, { 300, 240 });
            r.draw_text({"The quick brown fox jumps over the lazy dog", 32, rocket::rgb_color::black(), rGE__FONT_DEFAULT_MONOSPACED}, { 300, 240 + 32 });
            r.draw_text({"AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz", 32, color, rGE__FONT_DEFAULT_MONOSPACED}, { 300, 240 + 32 + 32 });
            r.draw_atlas_texture(am.get_texture(atlas_tx), { {350, 350}, { 256, 256 } }, { 16*12, 16*12 }, { 16, 16 });
            r.draw_texture(am.get_texture(arrow_tx), { { 350 + 256 + 96 + 8, 350 }, { 96, 96 } }, rotation);
            r.draw_polygon({ 350 + 256 + 96 + 8 + 96 / 2, 350 + 96 + 8 + 96 / 2 }, 96, color, 6, 30.f);

            if (rotation > 360.f) rotation = 0.f;
            if (r.get_framecount() % 10 == 0) {
                color.x = rand() % 255;
                color.y = rand() % 255;
                color.z = rand() % 255;
            }
        }
        r.draw_fps();
        r.end_frame();
        window.poll_events();

        rotation += 1;

        if (test_mode) return 0;
    }

    return 0;
}

DEFINE_PLATFORM_MAIN
