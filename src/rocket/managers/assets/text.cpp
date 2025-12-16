#include "../../include/rocket/asset.hpp"
#include <iostream>

namespace rocket {
    text_t::text_t(std::string text, float size, rgb_color color, std::shared_ptr<font_t> font) {
        this->text = text;
        this->size = size;
        this->color = color;
        this->font = font;
        if (this->font == nullptr) {
            this->font = font_t::font_default(size);
        } if (font.get() == reinterpret_cast<font_t*>(0x01)) {
            this->font = font_t::font_default_monospace(size);
        }
    }

    void text_t::set_size(float size) {
        this->size = size;
    }

    float text_t::get_size() {
        return this->size;
    }

    vec2f_t text_t::measure() {
        float x = 0.0f;
        float y = 0.0f;
        stbtt_aligned_quad q;
        for (char c : text) {
            if (c >= 32 && c <= 127) {
                stbtt_GetBakedQuad(font->cdata, 512, 512, c - 32, &x, &y, &q, 1);
            }
        }
        // for (const char* p = text.c_str(); *p; ++p) {
        //     if (*p >= 32 && *p < 128) {
        //         float x0;
        //         stbtt_GetBakedQuad(font->cdata, 512, 512, *p - 32, &x, nullptr, nullptr, 1);
        //     }
        // }
        return { x, size };
    }

    text_t::~text_t() {}
}
