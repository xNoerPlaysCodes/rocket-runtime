#ifndef RocketGL__HPP
#define RocketGL__HPP

#include <rocket/modularity/window_backend.hpp>
#ifndef GL_STATIC_DRAW 
#define GL_STATIC_DRAW 0x88E4
#endif
#include "rocket/types.hpp"
#include "glfnldr.hpp"
#include <utility>
#include <string>
#include <vector>
#include <rocket/macros.hpp>
#define rGL_TXID_INVALID ROCKETGE__InvalidNumber
#define rGL_SHADERLOC_INVALID -1 // OpenGL Specified do not change
#define rGL_SHADER_INVALID ROCKETGE__InvalidNumber
#define rGL_VAO_INVALID ROCKETGE__InvalidNumber
#define rGL_VBO_INVALID ROCKETGE__InvalidNumber
#define rGL_FBO_INVALID rgl::fbo_t{ ROCKETGE__InvalidNumber, ROCKETGE__InvalidNumber }

#define rGL_MAX_RECOMMENDED_DRAWCALLS 5000
#define rGL_MAX_RECOMMENDED_TRICOUNT 5000

// Unstable API & ABI, use at your own risk
namespace rgl {
    using vao_t = unsigned int;
    using vbo_t = unsigned int;

    using texture_id_t = unsigned int;
    using texture_unit_t = unsigned int;

    using shader_location_t = int;

    using shader_program_t = unsigned int;
    using cp_vert_shader_t = unsigned int;
    using cp_frag_shader_t = unsigned int;

    using blend_src_t = unsigned int;
    using blend_dst_t = unsigned int;

    struct fbo_t {
        unsigned int fbo;
        unsigned int color_tex;

        bool operator==(const fbo_t &other) const {
            return fbo == other.fbo;
        }

        bool operator!=(const fbo_t &other) const {
            return fbo != other.fbo;
        }
    };

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
        fbo_t bound_fbo;

        rgl::shader_program_t active_shader;

        blend_mode_t blend_mode;
    };

    struct texture_unit_handle_t {
        unsigned int unit = -1;
    };

    struct scoped_gl_texture_t {
    private:
        texture_unit_handle_t unit_handle;
        bool had_allocd_unit_handle = false;
    public:
        unsigned int id = rGL_TXID_INVALID;
    public:
        /// @brief Binds the texture to an available
        ///        texture unit (if available)
        /// @return int texture_unit (GL_TEXTUREn)
        ///         -1 if allocation for texture unit failed
        int bind();
    public:
        scoped_gl_texture_t();
        ~scoped_gl_texture_t();

        scoped_gl_texture_t(const scoped_gl_texture_t&) = delete;
        scoped_gl_texture_t& operator=(const scoped_gl_texture_t&) = delete;

        // move constructor
        scoped_gl_texture_t(scoped_gl_texture_t&& other) noexcept
            : id(other.id) {
            other.id = rGL_TXID_INVALID; // invalidate source
        }

        scoped_gl_texture_t& operator=(scoped_gl_texture_t&& other) noexcept {
            if (this != &other) {
                id = other.id;
                other.id = rGL_TXID_INVALID; // invalidate source
            }
            return *this;
        }
    };

    /// @brief Allocate Texture Unit
    /// @param dst Unit to write to
    /// @return bool success
    bool alloc_texture_unit(texture_unit_handle_t &dst);
    /// @brief Free Texture Unit
    /// @param dst Which unit
    void free_texture_unit(texture_unit_handle_t &dst);

    std::vector<std::string> init_gl(rocket::vec2f_t viewport_size, glfnldr::backend_t bkend = ROCKETGE__GLFNLDR_BACKEND_ENUM, rocket::window_backend_i *win = nullptr);
    void init_gl_wtd();
    std::pair<vao_t, vbo_t> compile_vo(
        const std::array<float, 12>& square_vertices = std::array<float,12>{
            0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f
        },
        unsigned int draw_type = GL_STATIC_DRAW,
        int stride_size = 2
    );

    std::pair<vao_t, vbo_t> compile_vo(
        const std::vector<float> &vertices,
        unsigned int draw_type = GL_STATIC_DRAW,
        int stride_size = 2
    );

    std::pair<vao_t, vbo_t> cache_compile_vo(
        std::string use,
        const std::array<float, 12>& square_vertices = std::array<float,12>{
            0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f
        },
        unsigned int draw_type = GL_STATIC_DRAW,
        int stride_size = 2 
    );

    std::pair<vao_t, vbo_t> cache_compile_vo(
        std::string use,
        const std::vector<float> &vertices,
        unsigned int draw_type = GL_STATIC_DRAW,
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
    void gl_viewport(const rocket::vec2f_t &offset, const rocket::vec2f_t &size);
    constexpr unsigned int gl_color_buffer_bit = 0x00004000;
    constexpr unsigned int gl_depth_buffer_bit = 0x00000100;
    constexpr unsigned int gl_texture0 = 0x84C0;
    void gl_clear(unsigned int gl_flags, rocket::rgba_color clear_color);

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

    /// @brief Use this as an alternative to glDrawArrays(...)
    /// @note Tracks Triangle Count and Drawcalls
    void gl_draw_arrays(unsigned int mode, int first, int count);

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

    void gl_uniform1f(shader_program_t prog, int location, float v0);
    void gl_uniform2f(shader_program_t prog, int location, float v0, float v1);
    void gl_uniform3f(shader_program_t prog, int location, float v0, float v1, float v2);
    void gl_uniform4f(shader_program_t prog, int location, float v0, float v1, float v2, float v3);

    void gl_uniform1i(shader_program_t prog, int location, int v0);
    void gl_uniform2i(shader_program_t prog, int location, int v0, int v1);
    void gl_uniform3i(shader_program_t prog, int location, int v0, int v1, int v2);
    void gl_uniform4i(shader_program_t prog, int location, int v0, int v1, int v2, int v3);

    void bind_texture_unit(rgl::texture_unit_t unit);
    void bind_texture(rgl::texture_id_t tx);

    void reset();
}

#endif
