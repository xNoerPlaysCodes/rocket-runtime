#ifndef ROCKETGE_QUARK_TYPES_HPP
#define ROCKETGE_QUARK_TYPES_HPP

#include "../rocket/types.hpp"
#include <ostream>

namespace quark {
    class percentage_t {
    private:
        double value;
    public:
        constexpr explicit percentage_t(double value);
        constexpr explicit percentage_t(int value);
        constexpr operator float() const;
        constexpr operator double() const;

        friend std::ostream& operator<<(std::ostream& os, const percentage_t& pct);
    };

    constexpr percentage_t operator"" _pct(long double val);

    class mass_t {
    private:
        double value;
    public:
        constexpr explicit mass_t(double value);
        constexpr explicit mass_t(int value);
        constexpr operator float() const;
        constexpr operator double() const;

        friend std::ostream& operator<<(std::ostream& os, const percentage_t& pct);
    };

    constexpr percentage_t operator"" _pct(long double val);

    struct material_t {
        float density;
        float elasticity;
        percentage_t friction = 15._pct;
        percentage_t restitution = 5._pct;

        rocket::vec2<float> force;
    };
}

#endif // ROCKETGE_QUARK_TYPES_HPP
