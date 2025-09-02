#include "rocket/runtime.hpp"
#include "rocket/shader.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <chrono>

int main() {
    rocket::window_t window({1280, 720}, "rgeExample - Custom Shader", {
        .resizable = true
    });
    rocket::renderer_2d r(&window, 60);
    rocket::shader_t shader = {rocket::shader_type::vert_frag, "resources/custom_shader.rlsl"};

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
