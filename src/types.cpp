#include "../include/rocket/types.hpp"

namespace rocket {
    bool ibounding_box::intersects(const ibounding_box& other) const {
        return
            pos.x < other.pos.x + other.size.x &&
            pos.x + size.x > other.pos.x &&
            pos.y < other.pos.y + other.size.y &&
            pos.y + size.y > other.pos.y;
    }

    bool fbounding_box::intersects(const fbounding_box& other) const {
        return
            pos.x < other.pos.x + other.size.x &&
            pos.x + size.x > other.pos.x &&
            pos.y < other.pos.y + other.size.y &&
            pos.y + size.y > other.pos.y;
    }

    bool ibounding_box_3d::intersects(const ibounding_box_3d& other) const {
        return
            pos.x < other.pos.x + other.size.x &&
            pos.x + size.x > other.pos.x &&
            pos.y < other.pos.y + other.size.y &&
            pos.y + size.y > other.pos.y &&
            pos.z < other.pos.z + other.size.z &&
            pos.z + size.z > other.pos.z;
    }

    
    
    
    bool fbounding_box_3d::intersects(const fbounding_box_3d& other) const {
        rocket::vec3f_t min_a = pos + size * rocket::vec3f_t{0.0f, 0.0f, 0.f}; // extend toward -Z
        rocket::vec3f_t max_a = pos + size;

        rocket::vec3f_t min_b = other.pos + other.size * rocket::vec3f_t{0.0f, 0.0f, 0};
        rocket::vec3f_t max_b = other.pos + other.size;

        return
            min_a.x < max_b.x && max_a.x > min_b.x &&
            min_a.y < max_b.y && max_a.y > min_b.y &&
            min_a.z < max_b.z && max_a.z > min_b.z;
    }




    template<typename T>
    vec2<T> vec2<T>::operator+(const vec2<T>& other) const {
        return {x + other.x, y + other.y};
    }

    template<typename T>
    vec2<T> vec2<T>::operator-(const vec2<T>& other) const {
        return {x - other.x, y - other.y};
    }

    template<typename T>
    vec2<T> vec2<T>::operator*(const vec2<T>& other) const {
        return {x * other.x, y * other.y};
    }

    template<typename T>
    vec2<T> vec2<T>::operator/(const vec2<T>& other) const {
        return {x / other.x, y / other.y};
    }

    template<typename T>
    vec3<T> vec3<T>::operator+(const vec3<T>& other) const {
        return {x + other.x, y + other.y, z + other.z};
    }

    template<typename T>
    vec3<T> vec3<T>::operator-(const vec3<T>& other) const {
        return {x - other.x, y - other.y, z - other.z};
    }

    template<typename T>
    vec3<T> vec3<T>::operator*(const vec3<T>& other) const {
        return {x * other.x, y * other.y, z * other.z};
    }

    template<typename T>
    vec3<T> vec3<T>::operator/(const vec3<T>& other) const {
        return {x / other.x, y / other.y, z / other.z};
    }

    template<typename T>
    vec4<T> vec4<T>::operator+(const vec4<T>& other) const {
        return {x + other.x, y + other.y, z + other.z, w + other.w};
    }

    template<typename T>
    vec4<T> vec4<T>::operator-(const vec4<T>& other) const {
        return {x - other.x, y - other.y, z - other.z, w - other.w};
    }

    template<typename T>
    vec4<T> vec4<T>::operator*(const vec4<T>& other) const {
        return {x * other.x, y * other.y, z * other.z, w * other.w};
    }

    template<typename T>
    vec4<T> vec4<T>::operator/(const vec4<T>& other) const {
        return {x / other.x, y / other.y, z / other.z, w / other.w};
    }
}
