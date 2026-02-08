#include "../include/rocket/runtime.hpp"
#include "../include/rocket/asset.hpp"
#include "../include/rocket/io.hpp"
#include "../include/rocket/renderer.hpp"
#include "../include/rocket/types.hpp"
#include "../include/rocket/window.hpp"
#include <algorithm>
#include <iostream>

int main(int argc, char **argv) {
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    // Initialize a native window
    rocket::window_t window = { {1280, 720}, "RocketGE - Initializer Test" };

    // Initialize default 2D renderer
    rocket::renderer_2d r2d(&window, 60, {.show_splash = !test_mode});

    // The main loop
    while (window.is_running()) {
        // Begin a new frame
        r2d.begin_frame();
        // Clear the screen with default color
        r2d.clear();
        {
            // Create and draw a basic rectangle
            r2d.draw_rectangle({
                // Set the position
                .pos = {
                    .x = 100,
                    .y = 100
                },
                // Set the size
                .size = {
                    .x = 128,
                    .y = 128
                }
                // Set the color
            }, rocket::rgba_color::red());
        }
        // End the frame
        r2d.end_frame();
        // Poll window events
        window.poll_events();

        if (test_mode) return 0;
    }

    window.close();
    r2d.close();
}
