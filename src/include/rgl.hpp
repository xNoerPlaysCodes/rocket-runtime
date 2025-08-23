#ifndef RocketGL__HPP
#define RocketGL__HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include "rocket/asset.hpp"
#include "rocket/types.hpp"
#include <string>
#include <vector>
// types
namespace rgl {
    using vao_t = GLuint;
    using vbo_t = GLuint;

    using texture_id_t = GLuint;

    using shader_program_t = GLuint;
    using cp_vert_shader_t = GLuint;
    using cp_frag_shader_t = GLuint;
    std::vector<std::string> init_gl(const rocket::vec2f_t viewport_size);

    shader_program_t get_paramaterized_quad(
        const rocket::vec2f_t &pos,
        const rocket::vec2f_t &size,
        const rocket::rgba_color &color,
        float rotation,
        float roundedness_radius
    );

    shader_program_t get_paramaterized_textured_quad(
        const rocket::vec2f_t &pos,
        const rocket::vec2f_t &size,
        float rotation,
        float roundedness_radius
    );

    std::pair<vao_t, vbo_t> get_text_vos();

    enum class shader_use_t {
        rect,
        text,
        textured_rect
    };

    void draw_shader(shader_program_t sp, shader_use_t use);

    void update_viewport(const rocket::vec2f_t &size);
}

#endif
