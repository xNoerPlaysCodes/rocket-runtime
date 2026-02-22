#ifndef RocketGL__HPP
#define RocketGL__HPP

#include "rocket/macros.hpp"
#ifdef ROCKETGE__Platform_Windows
#define GL_STATIC_DRAW 0x88E4
#endif
#include "rocket/asset.hpp"
#include "rocket/types.hpp"
#include <cstdint>
#include "glfnldr.hpp"
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

#ifdef RGL_EXPOSE_NATIVE_LIB
#include <GLFW/glfw3.h>
#endif

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

    struct scoped_gl_texture_t {
    private:
        std::unique_ptr<void, std::function<void(void*)>> cleanup;
    public:
        GLuint id = rGL_TXID_INVALID;
    public:
        scoped_gl_texture_t();
    };

    std::vector<std::string> init_gl(rocket::vec2f_t viewport_size, glfnldr::backend_t bkend = ROCKETGE__GLFNLDR_BACKEND_ENUM);
    void init_gl_wtd();
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

    struct draw_metrics_t {
        float avg_frametime = 0;
        float avg_fps = 0;

        float max_frametime = 0;
        float min_fps = 0;

        float min_frametime = 0;
        float max_fps = 0;
    };

    struct frame_metrics_t {
        int drawcalls = 0;
        int tricount = 0;
        int skipped_drawcalls = 0;
    };

    void update_draw_metrics_data(float frametime, float fps);
    draw_metrics_t get_draw_metrics();

    void update_frame_metrics_data(frame_metrics_t metrics);
    void add_frame_metrics_data_drawcalls(int);
    void add_frame_metrics_data_tricount(int);
    void add_frame_metrics_data_skipped_drawcalls(int);
    frame_metrics_t get_frame_metrics();
    void reset_frame_metrics();

    rgl::shader_program_t get_fxaa_simplified_shader();

    rgl::glstate_t save_state();
    void restore_state(rgl::glstate_t state);

    /// @brief Compiles all default shaders
    /// @note Can take a variable amount of time
    void compile_all_default_shaders();
}

#endif
