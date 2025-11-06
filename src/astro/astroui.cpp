#include "../../include/astro/astroui.hpp"
#include <memory>
#include <unordered_map>

namespace astro {
    rocket::renderer_2d *r;
    std::unordered_map<int, std::shared_ptr<rocket::font_t>> fonts;

    void set_renderer(rocket::renderer_2d *renderer) {
        r = renderer;
        fonts[24] = rocket::font_t::font_default(24);
        fonts[48] = rocket::font_t::font_default(48);
    }

    void begin_ui() {
        if (r == nullptr) {
            rocket::log_error("No renderer set for AstroUI", -3, "astro::begin_ui", "fatal");
            rocket::exit(0);
        }
    }

    void end_ui() {}

    void set_font(int size, std::shared_ptr<rocket::font_t> font) {
        fonts[size] = font;
    }

    rocket::rgba_color clr_gray() {
        return { 128, 128, 128, 255 };
    }

    rocket::rgba_color clr_lgray() {
        return { 192, 192, 192, 255 };
    }

    rocket::rgba_color clr_dgray() {
        return { 64, 64, 64, 255 };
    }

    rocket::rgba_color clr_blank() {
        return { 0, 0, 0, 0 };
    }

    rocket::rgb_color to_rgba(rocket::rgba_color color) {
        return { color.x, color.y, color.z };
    }

    button_t::button_t() {}
    button_t::button_t(std::string text, rocket::vec2f_t pos, rocket::vec2f_t size) {
        this->text = text;
        this->pos = pos;
        this->size = size;
    }

    bool button_t::is_hovering() {
        rocket::fbounding_box mpos = {
            .pos = rocket::io::mouse_pos_f(),
            .size = { 1, 1 }
        };
        return rocket::fbounding_box { .pos = pos, .size = size }.intersects(mpos);
    }

    bool button_t::is_clicking(bool hold, rocket::io::mouse_button button) {
        if (!is_hovering()) {
            return false;
        }
        if (hold) {
            return rocket::io::mouse_down(button);
        } else {
            return rocket::io::mouse_pressed(button);
        }
    }

    bool button_t::is_released(rocket::io::mouse_button button) {
        bool __last_frame_was_held = last_frame_was_held;
        last_frame_was_held = false;
        return is_hovering() && rocket::io::mouse_down(button) && __last_frame_was_held;
    }

    void button_t::draw(draw_info_t &info) {
        rocket::fbounding_box rect;
        rect.pos = pos;
        rect.size = size;

        if (info.border.width != 0) {
            rocket::fbounding_box border;
            border.pos = { pos.x - info.border.width, pos.y - info.border.width };
            border.size = { size.x + info.border.width * 2, size.y + info.border.width * 2 };

            if (info.border.radius == 0) {
                r->draw_rectangle(border, info.border.color);
            } else {
                r->draw_rectangle(border, info.border.color, 0, info.border.radius);
            }
        }

        if (info.border.radius == 0) {
            r->draw_rectangle(rect, info.color);
        } else {
            r->draw_rectangle(rect, info.color, 0, info.border.radius);
        }

        rocket::text_t text(this->text, 24, { info.text_color.x, info.text_color.y, info.text_color.z }, fonts[24]); // TODO font
        float text_offset_y = 0;
        r->draw_text(text, {pos.x + (size.x / 2) - (float) text.measure().x / 2 + text_offset, pos.y - text.font->get_line_height() / 2 + size.y / 2 + text_offset_y});
    }

    button_t::~button_t() {}

    input_field_t::input_field_t() {}
    input_field_t::input_field_t(std::string placeholder, rocket::vec2f_t pos, rocket::vec2f_t size) {
        this->placeholder = placeholder;
        this->pos = pos;
        this->size = size;
    }

    bool input_field_t::is_clicking(bool hold, rocket::io::mouse_button button) {
        if (!is_hovering()) {
            return false;
        }
        if (hold) {
            return rocket::io::mouse_down(button);
        } else {
            return rocket::io::mouse_pressed(button);
        }
    }

    bool input_field_t::is_hovering() {
        rocket::vec2f_t mouse_pos = rocket::io::mouse_pos_f();
        return rocket::fbounding_box { .pos = pos, .size = size }.intersects(rocket::fbounding_box { .pos = mouse_pos, .size = { 1, 1 } });
    }

    bool input_field_t::is_released(rocket::io::mouse_button button) {
        bool __last_frame_was_held = last_frame_was_held;
        last_frame_was_held = false;
        return is_hovering() && rocket::io::mouse_down(button) && __last_frame_was_held;
    }

    void input_field_t::draw(draw_info_t &info) { 
        rocket::fbounding_box rect;
        rect.pos = pos;
        rect.size = size;
        if (info.border.radius == 0) {
            r->draw_rectangle(rect, info.color);
        } else {
            r->draw_rectangle(rect, info.color, 0, info.border.radius);
        }

        if (info.border.width != 0) {
            rocket::fbounding_box border;
            border.pos = { pos.x - info.border.width, pos.y - info.border.width };
            border.size = { size.x + info.border.width * 2, size.y + info.border.width * 2 };
            if (info.border.radius == 0) {
                r->draw_rectangle(border, info.border.color);
            } else {
                r->draw_rectangle(border, info.border.color, 0, info.border.radius);
            }
        }

        if (info.border.radius == 0) {
            r->draw_rectangle(rect, info.color);
        } else {
            r->draw_rectangle(rect, info.color, 0, info.border.radius);
        }

        std::string draw_text = this->placeholder;
        rocket::rgb_color tc = {clr_dgray().x, clr_dgray().y, clr_dgray().z};
        if (this->text.length() > 0) {
            draw_text = this->text;
            tc = {info.text_color.x, info.text_color.y, info.text_color.z};
        }

        rocket::text_t txt(draw_text, 24, tc, fonts[24]);

        r->begin_scissor_mode(rect);
        {
            r->draw_text(txt, { pos.x + (size.x / 2) - (float) txt.measure().x / 2, pos.y - txt.font->get_line_height() / 2 + size.y / 2 });
        }
        r->end_scissor_mode();
    }

    void input_field_t::update_field_io(std::vector<int> printable_ascii_codes_allowed) {
        if (rocket::io::key_pressed(rocket::io::keyboard_key::backspace)) {
            if (!text.empty()) {
                text = text.substr(0, text.length() - 1);
            }
        }
        char key = rocket::io::get_formatted_char_typed();
        if (key != 0) {
            if (key >= 32 && key <= 126) {
                if (std::find(printable_ascii_codes_allowed.begin(), printable_ascii_codes_allowed.end(), key) != printable_ascii_codes_allowed.end()) {
                    text += static_cast<char>(key);
                }
            }
        }
    }

    void input_field_t::update_field_io(std::vector<field_type_t> types) {
        if (rocket::io::key_pressed(rocket::io::keyboard_key::backspace)) {
            if (!text.empty()) {
                text = text.substr(0, text.length() - 1);
            }
        }
        char key = rocket::io::get_formatted_char_typed();
        if (key != 0) {
            if (key >= 32 && key <= 126) {
                if (types.size() == 3) { // MAX_SIZE
                    text += static_cast<char>(key);
                    return;
                }
                if (std::find(types.begin(), types.end(), field_type_t::text) != types.end()) {
                    if ((key == 32) || (key >= 65 && key <= 90) || (key >= 97 && key <= 122)) {
                        text += static_cast<char>(key);
                    }
                } if (std::find(types.begin(), types.end(), field_type_t::number) != types.end()) {
                    if (key >= 48 && key <= 57) {
                        text += static_cast<char>(key);
                    }
                } if (std::find(types.begin(), types.end(), field_type_t::special_character) != types.end()) {
                    if ((key >= 58 && key <= 64) || (key >= 91 && key <= 96) || (key >= 123 && key <= 126)) {
                        text += static_cast<char>(key);
                    }
                }
            }
        }
    }

    bool input_field_t::is_submit() {
        return is_hovering() && rocket::io::key_pressed(rocket::io::keyboard_key::enter);
    }

    input_field_t::~input_field_t() {}

    dialog_t::dialog_t(std::string text, rocket::vec2f_t tpos, rocket::vec2f_t pos, rocket::vec2f_t size) {
        this->text = text;
        this->pos = pos;
        this->size = size;

        this->tpos = tpos;
    }

    dialog_t::dialog_t(std::string text, rocket::vec2f_t tpos, rocket::vec2f_t pos, rocket::vec2f_t size, button_t* button) {
        this->text = text;
        this->pos = pos;
        this->size = size;
        this->button_left = button;

        this->tpos = tpos;
    }

    dialog_t::dialog_t(std::string text, rocket::vec2f_t tpos, rocket::vec2f_t pos, rocket::vec2f_t size, button_t* button_left, button_t* button_right) {
        this->text = text;
        this->pos = pos;
        this->size = size;
        this->button_left = button_left;
        this->button_right = button_right;

        this->tpos = tpos;
    }

    void dialog_t::draw_self(draw_info_t &info) {
        rocket::fbounding_box rect;
        rect.pos = pos;
        rect.size = size;

        if (info.border.width != 0) {
            rocket::fbounding_box border;
            border.pos = { pos.x - info.border.width, pos.y - info.border.width };
            border.size = { size.x + info.border.width * 2, size.y + info.border.width * 2 };

            if (info.border.radius == 0) {
                r->draw_rectangle(border, info.border.color);
            } else {
                r->draw_rectangle(border, info.border.color, 0, info.border.radius);
            }
        }

        if (info.border.radius == 0) {
            r->draw_rectangle(rect, info.color);
        } else {
            r->draw_rectangle(rect, info.color, 0, info.border.radius);
        }

        rocket::text_t t(text, 24, { info.text_color.x, info.text_color.y, info.text_color.z }, fonts[24]);
        r->draw_text(t, tpos);
    }

    void dialog_t::draw(draw_info_t &info) {
        draw_self(info);
    }

    void dialog_t::draw(draw_info_t &info, draw_info_t &l) {
        draw_self(info);
        if (button_left != nullptr) {
            button_left->draw(l);
        }
    }

    void dialog_t::draw(draw_info_t &info, draw_info_t &l, draw_info_t &rt) {
        draw_self(info);
        if (button_left != nullptr) {
            button_left->draw(l);
        }

        if (button_right != nullptr) {
            button_right->draw(rt);
        }
    }

    dialog_t::~dialog_t() {
        this->button_left = nullptr;
        this->button_right = nullptr;
    }
}
