#ifndef RocketGL__HPP
#define RocketGL__HPP

#include "rocket/asset.hpp"
#include "rocket/types.hpp"
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <string>
#include <vector>
#define rGL_TXID_INVALID 0
#define rGL_SHADERLOC_INVALID -1
// types
namespace rgl {
    using vao_t = GLuint;
    using vbo_t = GLuint;

    using texture_id_t = GLuint;

    using shader_location_t = GLint;

    using shader_program_t = GLuint;
    using cp_vert_shader_t = GLuint;
    using cp_frag_shader_t = GLuint;
    std::vector<std::string> init_gl(const rocket::vec2f_t viewport_size);
    std::pair<vao_t, vbo_t> compile_vo(
        const std::array<float, 12>& square_vertices = std::array<float,12>{
            0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f
        },
        GLenum draw_type = GL_STATIC_DRAW,
        int stride_size = 2
    );

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
    std::pair<vao_t, vbo_t> get_quad_vos();
    std::pair<vao_t, vbo_t> get_txquad_vos();

    enum class shader_use_t {
        rect,
        text,
        textured_rect
    };

    void draw_shader(shader_program_t sp, shader_use_t use);
    void draw_shader(shader_program_t sp, vao_t vao, vbo_t vbo);

    void update_viewport(const rocket::vec2f_t &size);
    void update_viewport(const rocket::vec2f_t &offset, const rocket::vec2f_t &size);

    shader_program_t cache_compile_shader(const char *vs, const char *fs);
    shader_program_t nocache_compile_shader(const char *vs, const char *fs);

    shader_location_t get_shader_location(shader_program_t sp, const char *name);
    shader_location_t get_shader_location(shader_program_t sp, std::string name);

    rocket::vec2f_t get_viewport_size();

    void gl_draw_arrays(GLenum mode, GLint first, GLsizei count);

    int reset_drawcalls();
    int read_drawcalls();
}

#endif
