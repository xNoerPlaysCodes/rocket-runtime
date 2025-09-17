#include "../include/rocket/runtime.hpp"
#include "rocket/asset.hpp"
#include "rocket/renderer.hpp"
#include "rocket/rgl.hpp"
#include "rocket/window.hpp"
#include "rocket/macros.hpp"

int main() {
#ifdef ROCKETGE__Platform_Linux
    // wayland doesn't support windows icons!
    rocket::window_t::set_forced_platform(rocket::platform_type_t::linux_x11);
#endif
    rocket::window_t window = { { 1280, 720 }, "rGE - Window Icon Test", {
        .msaa_samples = 4
    } };
    rocket::renderer_2d r(&window);
    rgl::compile_all_default_shaders();
    rocket::asset_manager_t asset_manager;
    rocket::assetid_t texture = asset_manager.load_texture("resources/window_icon.jpg", rocket::texture_color_format_t::rgba);
    window.set_icon(asset_manager.get_texture(texture));

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            rocket::text_t text = { "Window Icon!", 36, rocket::rgb_color::black() };
            const auto text_size    = text.measure();
            const auto tex_size     = rocket::vec2{128, 128};
            const int padding       = 8;
            auto total_size = rocket::vec2{text_size.x + padding + tex_size.x,
                                           std::max(text_size.y, static_cast<float>(tex_size.y))};
            auto start = r.get_viewport_size() / 2 - total_size / 2;
            r.draw_text(text, start);
            auto tex_pos = start + rocket::vec2{text_size.x + padding, (total_size.y - tex_size.y)};
            r.draw_texture(asset_manager.get_texture(texture), { tex_pos, { static_cast<float>(tex_size.x), static_cast<float>(tex_size.y) } });
            r.draw_fps();
        }
        r.end_frame();
        window.poll_events();
    }

    r.close();
    window.close();
    return 0;
}
