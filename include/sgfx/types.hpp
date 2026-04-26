#pragma once

#include <rocket/types.hpp>

namespace sgfx {

    // vectors
    template<typename T>
    using vec2 = rocket::vec2<T>;

    template<typename T>
    using vec3 = rocket::vec3<T>;

    template<typename T>
    using vec4 = rocket::vec4<T>;

    // common float/int aliases
    using vec2i_t = rocket::vec2i_t;
    using vec2f_t = rocket::vec2f_t;
    using vec2d_t = rocket::vec2d_t;

    using vec3i_t = rocket::vec3i_t;
    using vec3f_t = rocket::vec3f_t;
    using vec3d_t = rocket::vec3d_t;

    using vec4i_t = rocket::vec4i_t;
    using vec4f_t = rocket::vec4f_t;
    using vec4d_t = rocket::vec4d_t;

    using vec2_t = rocket::vec2_t;
    using vec3_t = rocket::vec3_t;
    using vec4_t = rocket::vec4_t;

    using mat4 = rocket::mat4;

    using rgba_color = rocket::rgba_color;
    using rgb_color  = rocket::rgb_color;

    using ibounding_box = rocket::ibounding_box;
    using fbounding_box = rocket::fbounding_box;

    using ibounding_box_3d = rocket::ibounding_box_3d;
    using fbounding_box_3d = rocket::fbounding_box_3d;

    using float32_t = rocket::float32_t;
    using float64_t = rocket::float64_t;
}
