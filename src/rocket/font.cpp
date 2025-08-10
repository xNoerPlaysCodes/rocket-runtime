#include "../../include/rocket/asset.hpp"
#include "../../include/rocket/renderer.hpp"
#include <unordered_map>

namespace rocket {
    struct font_character_t {
        GLuint glid;

        vec2i_t size;
        vec2i_t bearing;
        int advance;
    };

    font_t::font_t() {}

    float font_t::get_line_height() const {
        return this->line_height;
    }

    // font_default() impl in asset.cpp

    void font_t::unload() {
        glDeleteTextures(1, &this->glid);
        this->glid = 0;
        this->line_height = 0;
    }

    font_t::~font_t() {
        this->unload();
    }
}
