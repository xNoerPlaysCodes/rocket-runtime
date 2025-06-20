#ifndef ROCKETGE__TYPES_HPP
#define ROCKETGE__TYPES_HPP

#include <cstdint>
namespace rocket {
    template<typename T>
    struct vec2 {
        T x, y;
    };

    template<typename T>
    struct vec3 {
        T x, y, z;
    };

    template<typename T>
    struct vec4 {
        T x, y, z, w;
    };

    /// @brief A draw-time casted 2D bounding box
    struct ibounding_box {
        vec2<int> pos;
        vec2<int> size;

        bool intersects(const ibounding_box& other) const;
    };

    /// @brief A floating-point 2D bounding box
    struct fbounding_box {
        vec2<float> pos;
        vec2<float> size;
    };

    
    using rgba_color    =       vec4<uint8_t>;
    using rgb_color     =       vec3<uint8_t>;

    using vec2i_t       =       vec2<int32_t>;
    using vec2f_t       =       vec2<float>;
    using vec2d_t       =       vec2<double>;

    using vec3i_t       =       vec3<int32_t>;
    using vec3f_t       =       vec3<float>;
    using vec3d_t       =       vec3<double>;

    using assetid_t     =       uint16_t;

    using ldouble_t     =       long double;
}

#endif
