# 🚀 Rocket
<img src="https://raw.githubusercontent.com/xNoerPlaysCodes/rocket-runtime/refs/heads/master/rocket_white_text_nobg.png">

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
- API Stability
    - A stable API with few breaking changes
- Human Language
    - Human-readable function names and classes with flow between each line

<b>and much more ...</b>

# Examples

## `basic window`
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

## `custom shader`
```cpp
#include "rocket/runtime.hpp"
#include "rocket/shader.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <chrono>

int main() {
    rocket::window_t window({1280, 720}, "rgeExample - Custom Shader");
    rocket::renderer_2d r(&window, 60);

    const char *vcode = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
    
        out vec2 fragPos;
    
        void main() {
            fragPos = aPos;
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )";
    const char *fcode = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 fragPos;

        vec3 hsv2rgb(float h, float s, float v) {
            vec3 k = vec3(5.0, 3.0, 1.0);
            vec3 p = abs(fract(h + k/6.0) * 6.0 - 3.0);
            return v * mix(vec3(1.0), clamp(p - 1.0, 0.0, 1.0), s);
        }

        void main() {
            float r = length(fragPos);
            float hue = atan(fragPos.y, fragPos.x) / (2.0 * 3.14159) + 0.5;

            float s = smoothstep(r - 1., 1., r);
            float v = s;

            FragColor = vec4(hsv2rgb(hue, s, v), 1.);
        }
    )";

    rocket::shader_t shader = {rocket::shader_type::vert_frag, vcode, fcode};

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            r.draw_shader(shader);
            r.draw_fps();
        }
        r.end_frame();
        window.poll_events();
    }

    r.close();
    window.close();
}
```
<img src="https://github.com/xNoerPlaysCodes/rocket-runtime/blob/master/custom_shader.png?raw=true">

<i>Custom Shader created with the 2D Renderer</i>
