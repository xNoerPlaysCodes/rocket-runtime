#include "rocket/asset.hpp"
#include "rocket/renderer.hpp"
#include "rocket/rgl.hpp"
#include <rocket/runtime.hpp>
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <rocket/threads.hpp>
#include <sstream>
#include <string>
#include <thread>

int main(int argc, char **argv) {
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::window_t window({ 1280, 720 }, "RocketGE - Multithreaded Test", {
        .resizable = false,
        .gl_version = {4,6},
        .gl_contextverifier = true,
    });
    rocket::renderer_2d r(&window, 60, {
            .show_splash = !test_mode
    });

    rocket::asset_manager_t am;
    std::shared_ptr<rocket::texture_t> tx = nullptr;

    std::thread t([&] () {
        std::string thread_id = std::to_string(rocket::thread_t::get_thread_id());
        rocket::log("simulating real work...", "thread", thread_id, "info");
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Load the file from disk on any thread
        tx = am.get_texture(am.load_texture("resources/window_icon.jpg"));
        rocket::log("Loaded texture", "thread", thread_id, "info");

        // Load the texture on the main thread
        rocket::thread_t::schedule([&r, &tx]() {
            std::string thread_id = std::to_string(rocket::thread_t::get_thread_id());
            rocket::log("Readying texture...", "thread", thread_id, "info");
            r.make_ready_texture(tx);
            rocket::log("Ready texture", "thread", thread_id, "info");
        });
    });

    t.detach();

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {       
            if (tx != nullptr && tx->is_ready()) {
                r.draw_texture(tx, { {250, 250}, {256,256} });
            }
            r.draw_rectangle({ 100, 100 }, { 256, 256 }, rocket::rgba_color::red());
            r.draw_fps(); // [FIXME] not rendering text when calling GL functions from another thread directly
        }
        r.end_frame();
        window.poll_events();
        if (test_mode) return 0;
    }
    window.close();
}
