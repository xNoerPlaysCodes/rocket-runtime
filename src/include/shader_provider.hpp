#ifndef ROCKETGE__SHADER_PROVIDER_HPP
#define ROCKETGE__SHADER_PROVIDER_HPP

#include <rgl.hpp>
#include <rocket/rgl.hpp>

namespace rocket {
    enum class shader_id_t : int {
        // Primitive
        rectangle = 0,
        textured_rectangle,
        circle_lines,
        text,

        // Screen Space
        fxaa,
        zoom,
    };

    rgl::shader_program_t get_shader(shader_id_t shid);
}

#endif//ROCKETGE__SHADER_PROVIDER_HPP
