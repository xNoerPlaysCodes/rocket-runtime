#include "rocket/renderer.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <chrono>
#include <iostream>
#include <rocket/runtime.hpp>
#include <rocket/scripting.hpp> 
#include <thread>

int main(int argc, char **argv) {
    rocket::init(argc, argv);
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::window_t window = { { 1280, 720 }, "RocketGE - Scripting Test" };
    rocket::renderer_2d r = { &window, 60, { .show_splash = !test_mode } };

    rocket::script::init();
    rocket::script::environment_t env;

    bool success = env.load_exec(std::filesystem::path("resources") / "update_rect_position.py");
    rocket::script::initialize_file_watcher([&env]() {
        rocket::log("Script changed, reloading...", "rocket::script", "initialize_file_watcher", "info");
        env.load_exec(std::filesystem::path("resources") / "update_rect_position.py");
    }, std::filesystem::path("resources") / "update_rect_position.py");

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            env.call("update");

            pybind11::object py_position = env.globals["cur_position"];
            rocket::vec2f_t position = py_position.cast<rocket::vec2f_t>();

            r.draw_rectangle(position, { 200, 200 });

            r.draw_fps();
        }
        r.end_frame();
        window.poll_events();

        if (test_mode) return !success;
    }
}
