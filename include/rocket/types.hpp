#ifndef ROCKETGE__TYPES_HPP
#define ROCKETGE__TYPES_HPP

#include <cstdint>
namespace rocket {
    template<typename T>
    struct vec2 {
        T x, y;

        vec2<T> operator+(const vec2<T>& other) const;
        vec2<T> operator-(const vec2<T>& other) const;
        vec2<T> operator*(const vec2<T>& other) const;
        vec2<T> operator/(const vec2<T>& other) const;
    };

    template<typename T>
    struct vec3 {
        T x, y, z;

        vec3<T> operator+(const vec3<T>& other) const;
        vec3<T> operator-(const vec3<T>& other) const;
        vec3<T> operator*(const vec3<T>& other) const;
        vec3<T> operator/(const vec3<T>& other) const;
    };

    template<typename T>
    struct vec4 {
        T x, y, z, w;

        vec4<T> operator+(const vec4<T>& other) const;
        vec4<T> operator-(const vec4<T>& other) const;
        vec4<T> operator*(const vec4<T>& other) const;
        vec4<T> operator/(const vec4<T>& other) const;
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

        bool intersects(const fbounding_box& other) const;
    };

    /// @brief A floating-point 3D bounding box
    struct fbounding_box_3d {
        vec3<float> pos;
        vec3<float> size;

        bool intersects(const fbounding_box_3d& other) const;
    };

    /// @brief An integer 3D bounding box
    struct ibounding_box_3d {
        vec3<int> pos;
        vec3<int> size;

        bool intersects(const ibounding_box_3d& other) const;
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
