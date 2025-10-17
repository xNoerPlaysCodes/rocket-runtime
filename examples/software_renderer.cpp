#include "rocket/renderer.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <cstring>
#include <iostream>
#include <limits>
#include <rocket/runtime.hpp>
#include <memory>
#include <rgl.hpp>

/// @brief Contiguous
using framebuffer_t = std::vector<rocket::rgba_color>;

rocket::rgba_color &access_framebuffer_pixel(framebuffer_t *fb, int x, int y, int width) {
    return fb->at(x + y * width);
}

/// @brief Chess Grid
void render(framebuffer_t *fb, rocket::renderer_2d *r) {
    int width = r->get_viewport_size().x;
    int height = r->get_viewport_size().y;

    static float t = 0.f;  // animation timer
    t += 5.f * r->get_delta_time();

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float nx = (float)x / width - 0.5f;
            float ny = (float)y / height - 0.5f;

            float value = std::sin(nx*50 + t) * std::cos(ny*50 + t);
            value = (value > 0) ? 1.f : 0.f;
            uint8_t col = static_cast<uint8_t>(value * 255);
            access_framebuffer_pixel(fb, x, y, width) = { 255, col, 255, 255 };
        }
    }
}


int main() {
    rocket::window_t window({1280, 720}, "rgeExample - Software Renderer", {
        .resizable = false
    });
    rocket::renderer_2d r(&window, -1);

    uint64_t i = 0;
    /// I use hyprland and hyprland likes to
    /// have animations and it takes a few
    /// frames to reach the actual window size
    ///
    /// @tldr remove this if not on hyprland
    constexpr uint64_t h = 4;
    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            if (i >= h) {
                framebuffer_t fb = r.get_framebuffer();
                render(&fb, &r);
                r.push_framebuffer(fb);
            }
            i++;
        }
        r.draw_fps();
        r.end_frame();
        window.poll_events();
    }
}
