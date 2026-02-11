#ifndef ROCKETGE_PHYSICS_HPP
#define ROCKETGE_PHYSICS_HPP

#include "types.hpp"

namespace quark_3d {
    struct still_in_development {};
}

namespace quark_2d {
    struct still_in_development {};

    void apply_gravity(quark::body_t &body, const quark::gravity_t &gravity, double dt);
}

#endif // ROCKETGE_PHYSICS_HPP
