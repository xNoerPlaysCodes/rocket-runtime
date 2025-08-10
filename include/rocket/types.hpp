#ifndef ROCKETGE__TYPES_HPP
#define ROCKETGE__TYPES_HPP

#include <cstdint>
#include <stdexcept>
namespace rocket {
    template<typename T>
    struct vec2 {
        T x, y;

        vec2<T> operator+(const vec2<T>& other) const {
            return { x + other.x, y + other.y };
        }
        vec2<T> operator-(const vec2<T>& other) const {
            return { x - other.x, y - other.y };
        }
        vec2<T> operator*(const vec2<T>& other) const {
            return { x * other.x, y * other.y };
        }
        vec2<T> operator/(const vec2<T>& other) const {
            return { x / other.x, y / other.y };
        }

        bool operator==(const vec2<T>& other) const {
            return x == other.x && y == other.y;
        }
    };

    template<typename T>
    struct vec3 {
        T x, y, z;

        vec3<T> operator+(const vec3<T>& other) const {
            return {x + other.x, y + other.y, z + other.z};
        }
        vec3<T> operator-(const vec3<T>& other) const {
            return {x - other.x, y - other.y, z - other.z};
        }
        vec3<T> operator*(const vec3<T>& other) const {
            return {x * other.x, y * other.y, z * other.z};
        }
        vec3<T> operator/(const vec3<T>& other) const {
            return {x / other.x, y / other.y, z / other.z};
        }

        bool operator==(const vec3<T>& other) const {
            return x == other.x && y == other.y && z == other.z;
        }
    };

    template<typename T>
    struct vec4 {
        T x, y, z, w;

        vec4<T> operator+(const vec4<T>& other) const {
            return {x + other.x, y + other.y, z + other.z, w + other.w};
        }
        vec4<T> operator-(const vec4<T>& other) const {
            return {x - other.x, y - other.y, z - other.z, w - other.w};
        }
        vec4<T> operator*(const vec4<T>& other) const {
            return {x * other.x, y * other.y, z * other.z, w * other.w};
        }
        vec4<T> operator/(const vec4<T>& other) const {
            return {x / other.x, y / other.y, z / other.z, w / other.w};
        }

        T& operator[](int i) {
            switch(i) {
                case 0: return x;
                case 1: return y;
                case 2: return z;
                case 3: return w;
                default: throw std::out_of_range("vec4 index out of range");
            }
        }

        // Const version (for read-only access)
        const T& operator[](int i) const {
            switch(i) {
                case 0: return x;
                case 1: return y;
                case 2: return z;
                case 3: return w;
                default: throw std::out_of_range("vec4 index out of range");
            }
        }

        bool operator==(const vec4<T>& other) const {
            return x == other.x && y == other.y && z == other.z && w == other.w;
        }
    };
    
    struct mat4 {
        vec4<float> columns[4];

        inline static mat4 identity() {
            return {
                .columns = {
                    {1, 0, 0, 0},
                    {0, 1, 0, 0},
                    {0, 0, 1, 0},
                    {0, 0, 0, 1}
                }
            };
        }

        inline static mat4 zero() {
            return {
                .columns = {
                    {0, 0, 0, 0},
                    {0, 0, 0, 0},
                    {0, 0, 0, 0},
                    {0, 0, 0, 0}
                }
            };
        }

        static mat4 from_array(const float* arr) {
            mat4 result;
            for (int i = 0; i < 4; i++)
                for (int j = 0; j < 4; j++)
                    result.columns[i][j] = arr[i * 4 + j];
            return result;
        }

        inline mat4 operator*(const mat4& other) const {
            mat4 result = zero();
            for (int col = 0; col < 4; ++col) {
                for (int row = 0; row < 4; ++row) {
                    float sum = 0.0f;
                    for (int k = 0; k < 4; ++k) {
                        sum += columns[k][row] * other.columns[col][k];
                    }
                    result.columns[col][row] = sum;
                }
            }
            return result;
        }

        inline mat4 translate(const vec3<float>& v) const {
            mat4 t = identity();
            t.columns[3][0] = v.x;
            t.columns[3][1] = v.y;
            t.columns[3][2] = v.z;
            return (*this) * t;
        }

        inline mat4 scale(const vec3<float>& v) const {
            mat4 s = identity();
            s.columns[0][0] = v.x;
            s.columns[1][1] = v.y;
            s.columns[2][2] = v.z;
            return (*this) * s;
        }

        inline mat4 transpose() const {
            mat4 result;
            for (int col = 0; col < 4; ++col)
                for (int row = 0; row < 4; ++row)
                    result.columns[col][row] = columns[row][col];
            return result;
        }

        inline const float* data() const {
            return &columns[0].x;
        }
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

    struct rgba_color {
        uint8_t x, y, z, w;

        static rgba_color white();
        static rgba_color black();
        static rgba_color red();
        static rgba_color green();
        static rgba_color blue();
        static rgba_color yellow();
    };

    struct rgb_color {
        uint8_t x, y, z;

        static rgb_color white();
        static rgb_color black();
        static rgb_color red();
        static rgb_color green();
        static rgb_color blue();
        static rgb_color yellow();
    };

    using vec2i_t       =       vec2<int32_t>;
    using vec2f_t       =       vec2<float>;
    using vec2d_t       =       vec2<double>;

    using vec3i_t       =       vec3<int32_t>;
    using vec3f_t       =       vec3<float>;
    using vec3d_t       =       vec3<double>;

    using vec4i_t       =       vec4<int32_t>;
    using vec4f_t       =       vec4<float>;
    using vec4d_t       =       vec4<double>;

    using vec2_t = vec2f_t;
    using vec3_t = vec3f_t;
    using vec4_t = vec4f_t;

    using assetid_t     =       uint16_t;

    using ldouble_t     =       long double;
}

#endif
