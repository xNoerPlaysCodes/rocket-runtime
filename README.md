# 🚀 Rocket

<img src="https://raw.githubusercontent.com/xNoerPlaysCodes/rocket-runtime/refs/heads/master/rocket_white_text_nobg.png">

`basic window`
```cpp
#include <rocket/runtime.hpp>

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
```
<img src="https://github.com/xNoerPlaysCodes/rocket-runtime/blob/master/basic_window.png?raw=true">
<i>A basic window created with the above code</i>

# Included Libraries
- Rocket
    - The core rocket runtime, for drawing logic and rendering with GLFW and OpenGL.

- Astro
    - The All-in-one GUI library for making simple but powerful game GUIs.

- Quark
    -  [unfinished] The simple but powerful 2D + 3D Physics Library for your Physic(al) needs.

# Features
- Asset Manager
    - A powerful and clean asset manager with use timeouts and lazy loading

<b>and much more ...</b>
