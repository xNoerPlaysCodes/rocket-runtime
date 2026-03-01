#include "rocket/asset.hpp"
#include "rocket/types.hpp"
#include <editor/page.hpp>
#include <astro/astroui.hpp>

void rgeditor::draw_page_create_project(__RGEDITOR_PAGE_ARGS, rocket::assetid_t bg_tx_id) {
    astro::draw_info_t info = {
        .color = clr_gray(),
        .text_color = rocket::rgb_color::black(),
        .border = {
            .width = 0,
            .radius = 0.25,
            .color = clr_blank()
        }
    };

    float base = 56;
    static rocket::text_t editor_text = { "RocketGE - Editor", 32, rocket::rgb_color::white(), rGE__FONT_DEFAULT_MONOSPACED };
    static astro::button_t new_button = { "New", { 8 + 8, base + 8 + 8 }, { 300 - 16, 75 } };
    static astro::button_t open_button = { "Open", { 8 + 8, base + 75 + 8*3 }, { 300 - 16, 75 } };
    static astro::dialog_t box = { "", rocket::vec2f_t(), { 8, 8 }, { 300, 720 - 16 } };

    r.draw_texture(am.get_texture(bg_tx_id), { {0,0}, r.get_viewport_size() });

    // Change sizes to match
    box.size.y = r.get_viewport_size().y - 16;
}
