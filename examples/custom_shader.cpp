#include "rocket/runtime.hpp"
#include "rocket/shader.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <chrono>

// Make sure to use rocket_main instead of main
// with extra rocket_arguments_t argument
int rocket_main(int argc, char **argv, rocket_arguments_t) {
    rocket::init(argc, argv);
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

// RocketGE will handle platform-specific main functions for you
// and make your game cross-platform!
DEFINE_PLATFORM_MAIN

// #include "rocket/runtime.hpp"
// #include "rocket/shader.hpp"
// #include "rocket/types.hpp"
// #include "rocket/window.hpp"
// #include <chrono>
//
// int main() {
//     rocket::window_t window({1280, 720}, "rgeExample - Custom Shader", {
//         .resizable = true
//     });
//     rocket::renderer_2d r(&window, 60);
//     rocket::shader_t shader = {rocket::shader_type::vert_frag, "resources/custom_shader.rlsl"};
//
//     while (window.is_running()) {
//         r.begin_frame();
//         r.clear();
//         {
//             r.draw_shader(shader);
//             r.draw_fps();
//         }
//         r.end_frame();
//         window.poll_events();
//     }
//
//     r.close();
//     window.close();
// }
