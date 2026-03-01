#ifndef EDITOR__PAGE_HPP
#define EDITOR__PAGE_HPP

#include "astro/astroui.hpp"
#include "rocket/asset.hpp"
#include "rocket/renderer.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <functional>

using button_callback_t = std::function<void(const astro::button_t &button, std::string page)>;

#define __RGEDITOR_PAGE_ARGS rocket::window_t &window, rocket::renderer_2d &r, rocket::asset_manager_t &am, button_callback_t callback

namespace rgeditor {
    void draw_page_main_menu (__RGEDITOR_PAGE_ARGS, rocket::assetid_t bg_tx_id);
    void draw_page_create_project (__RGEDITOR_PAGE_ARGS, rocket::assetid_t bg_tx_id);

    inline rocket::rgba_color clr_gray() {
        return { 128, 128, 128, 255 };
    }

    inline rocket::rgba_color clr_lgray() {
        return { 192, 192, 192, 255 };
    }

    inline rocket::rgba_color clr_dgray() {
        return { 64, 64, 64, 255 };
    }

    inline rocket::rgba_color clr_blank() {
        return { 0, 0, 0, 0 };
    }

    inline void draw_info_util(astro::button_t &button, astro::draw_info_t &info) {
        if (button.is_clicking(true)) {
            info.color = rocket::rgba_color::black();
            info.text_color = rocket::rgb_color::white();
        } else if (button.is_hovering()) {
            info.color = clr_dgray();
            info.text_color = rocket::rgb_color::white();
        } else {
            info.color = clr_gray();
            info.text_color = rocket::rgb_color::black();
        }
    }
}

#endif//EDITOR__PAGE_HPP
