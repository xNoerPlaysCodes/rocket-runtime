#include "rocket/window.hpp"
#include <iostream>
#include <rocket/runtime.hpp>
#include <rocket/asset.hpp>
#include <rocket/audio.hpp>

int main(int argc, char **argv) {
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::set_cli_arguments(argc, argv);
    rocket::window_t window = { { 1280, 720 }, "RocketGE - Sound Engine Test" };
    rocket::renderer_2d r(&window, 60, {.show_splash = !test_mode});

    std::vector<rocket::audio::device_t> devices = rocket::audio::get_devices();

    rocket::asset_manager_t am(std::chrono::seconds(1));
    rocket::log("Loading sound...", "main.cpp", "main", "info");
    auto sound = am.get_sound(am.load_sound("resources/output.ogg"));

    rocket::audio::sound_engine_t se(rocket::audio::device_t::get_default());
    se.play(*sound);

    // auto stream_sound = se.stream("/home/noerlol/C-Projects/RocketGE/bin/resources/output.ogg");
    // se.update_music_streams();
    // se.play(stream_sound);

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            r.draw_fps();
        }
        r.end_frame();
        window.poll_events();
        if (test_mode) return 0;
    }

    am.close();
}
