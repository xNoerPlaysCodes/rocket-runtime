#include "../../include/rocket/asset.hpp"
#include <internal_types.hpp>

extern "C" { void glDeleteTextures(int n, const unsigned int *textures); }

namespace rocket {
    struct font_character_t {
        unsigned int glid;

        vec2i_t size;
        vec2i_t bearing;
        int advance;
    };

    font_t::font_t() {
        this->cdata = new rocket::internal_cdata;
    }

    float font_t::get_line_height() const {
        return this->line_height;
    }

    // font_default() impl in asset.cpp
    // font_default_monospaced() impl in asset.cpp

    void font_t::unload() {
        this->hdl = 0;
    }

    void font_t::set_unloaded() {
        this->loaded = false;
    }

    void font_t::reload() {
        this->loaded = true;
    }

    font_t::~font_t() {
        this->unload();
    }
}
