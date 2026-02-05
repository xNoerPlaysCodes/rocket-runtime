#include <rocket/runtime.hpp>
#include <rocket/shader.hpp>
#include <shader_provider.hpp>
#include <unordered_map>
#include <resources/shader_fxaa.h>
#include <resources/shader_rectangle.h>
#include <resources/shader_text.h>
#include <resources/shader_textured_rectangle.h>
#include <resources/shader_circle_lines.h>

namespace rocket {
    static thread_local std::unordered_map<shader_id_t, shader_t> shader_map;

    rgl::shader_program_t get_shader(shader_id_t shid) {
        if (shader_map.find(shid) != shader_map.end()) {
            return shader_map[shid].glprogram;
        }


#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

#define LOAD_SHADER(name) { \
    shader_map[shid] = shader_t::load_from_rlsl_source(\
        shader_type::vert_frag, \
        rocket_resource::CONCAT(shader_, CONCAT(name, _rlsl))); \
        return shader_map[shid].glprogram; \
    }

        if (shid == shader_id_t::rectangle) {
            LOAD_SHADER(rectangle);
        }
        else if (shid == shader_id_t::textured_rectangle) {
            LOAD_SHADER(textured_rectangle);
        }
        else if (shid == shader_id_t::text) {
            LOAD_SHADER(text);
        }
        else if (shid == shader_id_t::fxaa) {
            LOAD_SHADER(fxaa);
        }
        else if (shid == shader_id_t::circle_lines) {
            LOAD_SHADER(circle_lines);
        }
        else {
            rocket::log("invalid shader_id given", "rocket", "get_shader", "error");
            return rGL_SHADER_INVALID;
        }
    }
}
