#ifndef ROCKETGE__INTL_INTERNAL_TYPES_HPP
#define ROCKETGE__INTL_INTERNAL_TYPES_HPP

#include "lib/stb/stb_truetype.h"
#include <rocket/window.hpp>

namespace rocket {
    struct internal_cdata {
        stbtt_bakedchar a[96];
    };

    struct native_window_t {
        void *w;
        window_backend_t backend;
    };
}

#endif//ROCKETGE__INTL_INTERNAL_TYPES_HPP
