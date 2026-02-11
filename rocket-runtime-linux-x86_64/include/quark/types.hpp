#ifndef ROCKETGE_QUARK_TYPES_HPP
#define ROCKETGE_QUARK_TYPES_HPP

#include "../rocket/types.hpp"
#include <ostream>

namespace quark {
    class percentage_t {
    private:
        double value;
    public:
        constexpr explicit percentage_t(double value) {
            this->value = value;
        }
        constexpr explicit percentage_t(int value) {
            this->value = value;
        }
        constexpr operator float() const {
            return static_cast<float>(value);
        }
        constexpr operator double() const {
            return value;
        }

        friend std::ostream& operator<<(std::ostream& os, const percentage_t& pct) {
            return os << pct.value;
        }
    };

    constexpr percentage_t operator""_pct(long double val) {
        return percentage_t(static_cast<double>(val));
    }

    struct material_t {
        float density = 0.25f;
        float elasticity = 0.15f;
        percentage_t friction = 15._pct;
        percentage_t restitution = 5._pct;

        rocket::vec2<float> force = rocket::vec2<float>::v(0.f);
    };

    struct gravity_t {
        rocket::vec2<float> force = rocket::vec2<float>::v(00.f);
    };

    struct movement_t {
        rocket::vec2f_t momentum = rocket::vec2f_t::v(0.f);
        rocket::vec2f_t velocity = rocket::vec2f_t::v(0.f);
        rocket::vec2f_t position = rocket::vec2f_t::v(0.f);
    };

    struct body_t {
        double mass = 0.f;
        material_t material;
        movement_t movement;
    };
}

#endif // ROCKETGE_QUARK_TYPES_HPP
