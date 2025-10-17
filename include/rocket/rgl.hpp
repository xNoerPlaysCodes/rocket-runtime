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
#define rGL_SHADER_INVALID 0
#define rGL_VAO_INVALID 0
#define rGL_VBO_INVALID 0
#define rGL_FBO_INVALID rgl::fbo_t{ 0, 0 }

#define rGL__FEATURE_SUPPORT_FBO

#define rGL_MAX_RECOMMENDED_DRAWCALLS 5000
#define rGL_MAX_RECOMMENDED_TRICOUNT 5000
// types
namespace rgl {
    using vao_t = GLuint;
    using vbo_t = GLuint;

    using texture_id_t = GLuint;
    using texture_unit_t = GLint;

    using shader_location_t = GLint;

    using shader_program_t = GLuint;
    using cp_vert_shader_t = GLuint;
    using cp_frag_shader_t = GLuint;

    using blend_src_t = GLenum;
    using blend_dst_t = GLenum;

#ifdef rGL__FEATURE_SUPPORT_FBO
    struct fbo_t {
        GLuint fbo;
        GLuint color_tex;

        bool operator==(const fbo_t &other) const {
            return fbo == other.fbo;
        }

        bool operator!=(const fbo_t &other) const {
            return fbo != other.fbo;
        }
    };
#endif

    struct blend_mode_t {
        blend_src_t src_rgb;
        blend_dst_t dst_rgb;

        blend_src_t src_alpha;
        blend_dst_t dst_alpha;

        bool enabled;
    };

    struct glstate_t {
        rgl::fbo_t bound_framebuffer;
        rgl::texture_unit_t bound_texture_unit;
        std::pair<rgl::vao_t, rgl::vbo_t> bound_vo;

        rgl::shader_program_t active_shader;

        blend_mode_t blend_mode;
    };


    std::vector<std::string> init_gl(const rocket::vec2f_t viewport_size);
    std::pair<vao_t, vbo_t> compile_vo(
        const std::array<float, 12>& square_vertices = std::array<float,12>{
            0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f
        },
        GLenum draw_type = GL_STATIC_DRAW,
        int stride_size = 2
    );

    std::pair<vao_t, vbo_t> compile_vo(
        const std::vector<float> &vertices,
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

    shader_program_t get_shader(shader_use_t);

    void draw_shader(shader_program_t sp, shader_use_t use);
    void draw_shader(shader_program_t sp, vao_t vao, vbo_t vbo);

    void update_viewport(const rocket::vec2f_t &size);
    void update_viewport(const rocket::vec2f_t &offset, const rocket::vec2f_t &size);

    shader_program_t cache_compile_shader(const char *vs, const char *fs);
    shader_program_t nocache_compile_shader(const char *vs, const char *fs);

    shader_location_t get_shader_location(shader_program_t sp, const char *name);
    shader_location_t get_shader_location(shader_program_t sp, std::string name);

    fbo_t create_fbo();
    void use_fbo(fbo_t fbo);
    void reset_to_default_fbo();
    void delete_fbo(fbo_t fbo);
    bool is_active_any_fbo();
    fbo_t get_active_fbo();

    rocket::vec2f_t get_viewport_size();

    void gl_draw_arrays(GLenum mode, GLint first, GLsizei count);

    int reset_drawcalls();
    int read_drawcalls();

    int reset_tricount();
    int read_tricount();

    // TODO Implement!
    struct draw_metrics_t {
    };

    rgl::shader_program_t get_fxaa_simplified_shader();

    rgl::glstate_t save_state();
    void restore_state(rgl::glstate_t state);

    /// @brief Compiles all default shaders
    /// @note Can take a variable amount of time
    void compile_all_default_shaders();
}

#endif
