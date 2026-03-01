#include "rocket/asset.hpp"
#include "rocket/types.hpp"
#include <editor/page.hpp>
#include <astro/astroui.hpp>

void rgeditor::draw_page_main_menu(__RGEDITOR_PAGE_ARGS, rocket::assetid_t id) {
    r.draw_texture(am.get_texture(id), { {0,0}, r.get_viewport_size() });

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
    
    astro::draw_info_t new_button_info = info;
    astro::draw_info_t open_button_info = info;
    info.color = { 14, 14, 14 };
    info.border.radius = 0.05;
    astro::draw_info_t dialog_info = info;
    draw_info_util(new_button, new_button_info);
    draw_info_util(open_button, open_button_info);
    box.draw(dialog_info);
    new_button.draw(new_button_info);
    open_button.draw(open_button_info);
    r.draw_text(editor_text, { 32, 24 });

    if (new_button.is_clicking(true)) {
        callback(new_button, "main_menu");
    }
    if (open_button.is_clicking(true)) {
        callback(open_button, "main_menu");
    }

    // Change sizes to match
    box.size.y = r.get_viewport_size().y - 16;
}
