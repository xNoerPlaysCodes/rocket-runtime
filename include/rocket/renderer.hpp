#ifndef ROCKETGE__RENDERER_HPP
#define ROCKETGE__RENDERER_HPP

#include "macros.hpp"
#include <cstdint>
#include "glfnldr.hpp"
#include "asset.hpp"
#include "rocket/rgl.hpp"
#include "types.hpp"
#include "window.hpp"
#include "shader.hpp"
#include <chrono>
#include <memory>
#include <vector>
#include "glm/fwd.hpp"

namespace rocket {
    struct instanced_quad_t {
        vec2f_t pos = {0,0};
        vec2f_t size = {0,0};

        unsigned int gltxid = 0;
        rgba_color color = {0,0,0,0};
    };

    class renderer_2d;  

    struct camera_2d;
    struct renderer_2d_impl_t;

    // RENDERER_2D_HERE

    /// @brief 2D camera
}

#endif
