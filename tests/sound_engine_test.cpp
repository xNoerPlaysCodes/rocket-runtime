#include "rocket/window.hpp"
#include <iostream>
#include <rocket/runtime.hpp>
#include <rocket/asset.hpp>
#include <rocket/audio.hpp>

int main(int argc, char **argv) {
    rocket::set_cli_arguments(argc, argv);
    rocket::window_t window = { { 1280, 720 }, "RocketGE - Sound Engine Test" };
    rocket::renderer_2d r(&window, 60);

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
    }

    am.close();
}
