#include "../include/rocket/runtime.hpp"
#include "rocket/asset.hpp"
#include "rocket/renderer.hpp"
#include "rocket/window.hpp"

int main() {
    rocket::window_t window = { { 1280, 720 }, "rGE - Window Icon Test" };
    rocket::renderer_2d r(&window);
    rocket::asset_manager_t asset_manager;
    rocket::assetid_t texture = asset_manager.load_texture("resources/window_icon.jpg", rocket::texture_color_format_t::rgba);
    window.set_icon(asset_manager.get_texture(texture));

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            rocket::text_t text = { "Window Icon!", 36, rocket::rgb_color::black() };
            auto text_size = text.measure();
            auto tex_size  = rocket::vec2{128, 128};
            int padding    = 8; // space between text and texture

            // total group width = text + padding + texture
            auto total_size = rocket::vec2{text_size.x + padding + tex_size.x,
                                           std::max(text_size.y, static_cast<float>(tex_size.y))};

            // start position = center - half of group
            auto start = r.get_viewport_size() / 2 - total_size / 2;

            // draw text at start
            r.draw_text(text, start);

            // draw texture after text + padding
            auto tex_pos = start + rocket::vec2{text_size.x + padding, (total_size.y - tex_size.y)};
            r.draw_texture(asset_manager.get_texture(texture), { tex_pos, { static_cast<float>(tex_size.x), static_cast<float>(tex_size.y) } });
        }
        r.end_frame();
        window.poll_events();
    }

    r.close();
    window.close();
    return 0;
}
