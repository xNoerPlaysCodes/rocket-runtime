#ifndef ROCKETGE__SHADER_PROVIDER_HPP
#define ROCKETGE__SHADER_PROVIDER_HPP

#include <rgl.hpp>
#include <rocket/rgl.hpp>
#include <internal_types.hpp>

namespace rocket {
    enum class shader_id_t : int {
        // Primitive
        rectangle = 0,
        textured_rectangle,
        atlas_textured_rectangle,
        circle_lines,
        text,
        polygon,

        // Screen Space
        fxaa,
        zoom,
    };

    rgl::shader_program_t gl_get_shader(shader_id_t shid);
    vk_shader_t vk_get_shader(shader_id_t shid);
    void shader_provider_compile_all_gl();
    void shader_provider_compile_all_vk();
    void shader_provider_reset();
}

#endif//ROCKETGE__SHADER_PROVIDER_HPP
