#include "../../include/quark/physics.hpp"
#include "../../include/quark/types.hpp"

namespace quark_2d {
    void apply_gravity(quark::body_t &body, const quark::gravity_t &gravity, double dt) {
        // acceleration = gravity
        body.movement.velocity.x += gravity.force.x * dt;
        body.movement.velocity.y += gravity.force.y * dt;

        // update position
        body.movement.position.x += body.movement.velocity.x * dt;
        body.movement.position.y += body.movement.velocity.y * dt;
    }
}
