#include "../include/rocket/runtime.hpp"
#include "rocket/renderer.hpp"
#include "rocket/shader.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"

int main() {
    rocket::window_t window({ 1280, 720 }, "RocketGE - Shader Test");
    rocket::renderer_2d r(&window);

    rocket::shader_t shader = rocket::shader_t::rectangle({ 255, 0, 0, 255 });

    rocket::log("Shader test doesn't work", "main.cpp", "main", "info");

    while (window.is_running()) {
        r.begin_frame();
        r.clear(rocket::rgba_color::yellow());
        {
            r.draw_shader(shader, { { 0, 0 }, { 128, 128 } });
            // r.draw_rectangle({ { 0, 0 }, { 128, 128 } }, { 255, 0, 0, 255 });
        }
        r.end_frame();
        window.poll_events();
    }

    r.close();
    window.close();
}
