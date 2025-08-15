#include "../include/rocket/runtime.hpp"
#include "../include/rocket/asset.hpp"
#include "../include/rocket/io.hpp"
#include "../include/rocket/renderer.hpp"
#include "../include/rocket/types.hpp"
#include "../include/rocket/window.hpp"
#include <algorithm>
#include <iostream>

int main() {
    // Initialize a native window
    rocket::window_t window = { {1280, 720}, "Basic Window" };

    // Initialize default 2D renderer
    rocket::renderer_2d r2d(&window);

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
    }

    window.close();
    r2d.close();
}
