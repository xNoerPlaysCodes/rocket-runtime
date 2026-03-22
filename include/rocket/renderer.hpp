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
    constexpr int fps_uncapped = -1;
    struct instanced_quad_t {
        vec2f_t pos = {0,0};
        vec2f_t size = {0,0};

        unsigned int gltxid = 0;
        rgba_color color = {0,0,0,0};
    };
    struct renderer_flags_t {
        bool share_renderer_as_global = true;
        /// @brief Show the splash screen
        bool show_splash = true;
        /// @brief (Advanced) Change Glfnldr backend if available
        /// @note List available backends with glfnldr::get_backends()
        glfnldr::backend_t glfnldr_backend = ROCKETGE__GLFNLDR_BACKEND_ENUM;
    };
    enum class render_mode_t {
        texture_filter_none,
        camera,
    };

    struct graphics_settings_t {
        bool viewport_visibility_checks = true;
    };

    class renderer_2d;
    
    struct render_cache_t {
    private:
        rgl::fbo_t fbo;
        friend class renderer_2d;
    public:
        std::function<void(rocket::renderer_2d *ren)> draw = nullptr;
    public:
        /// @brief Get FB Color Texture
        /// @note Call after end_render_cache ONLY
        unsigned int get_texture();
    };

    struct camera_2d;
    struct renderer_2d_impl_t;

    class renderer_2d {
    private:
        window_backend_i *window = nullptr;
        camera_2d *cam = nullptr;
        int fps = 60;
        bool wireframe = false;
        bool vsync = false;

        using clock = std::chrono::steady_clock;
        template<typename Clock>
        using time_point = std::chrono::time_point<Clock>;

        time_point<clock> frame_start_time;
        time_point<clock> last_time;
        double delta_time;

        uint64_t frame_counter = 0;

        bool frame_started = false;

        rocket::vec2f_t override_viewport_size = {-1, -1};
        rocket::vec2f_t override_viewport_offset = {-1, -1};

        std::vector<render_mode_t> active_render_modes;
        
        renderer_flags_t flags;

        bool splash_shown = false;

        graphics_settings_t graphics_settings;

        renderer_2d_impl_t *impl = nullptr;

        friend class renderer_3d;
        friend class font_t;
    private:
        enum class gfx_chk_result {
            not_drawable,
            drawable,
        };
        gfx_chk_result check_graphics_settings(rocket::vec2f_t pos, rocket::vec2f_t sz);
    public:
        /// @brief Check if frame has begun
        bool has_frame_began();
        /// @brief Begin frame
        void begin_frame();
        /// @brief Show the splash ignoring flags
        void show_splash();
        /// @brief Begin render mode
        void begin_render_mode(render_mode_t);
        /// @brief Get a contiguous block of pixels
        /// @brief adjusted to viewport size
        /// @brief VERY SLOW use only for SOFTWARE rendering
        std::vector<rgba_color> get_framebuffer();
        /// @brief Push a contiguous block of pixels
        /// @brief adjusted to viewport size
        /// @brief ONLY for SOFTWARE rendering
        void push_framebuffer(const std::vector<rgba_color> &framebuffer);
        /// @brief Get the size of the viewport
        vec2f_t get_viewport_size();
        /// @brief Begin scissor mode
        void begin_scissor_mode(rocket::fbounding_box rect);
        /// @brief Begin scissor mode
        void begin_scissor_mode(rocket::vec2f_t pos, rocket::vec2f_t size);
        /// @brief Begin scissor mode
        void begin_scissor_mode(float x, float y, float sx, float sy);
        /// @brief Clear the screen
        /// @note uses OpenGL Flags: GL_CLEAR_COLOR_BUFFER_BIT,
        /// @note and GL_CLEAR_DEPTH_BUFFER_BIT
        void clear(rocket::rgba_color color = { 255, 255, 255, 255 });

        /// @brief Draw a shader
        /// @note Uniforms to be set in the shader by shader.set_uniform(...)
        void draw_shader(shader_t shader);

        /// @brief Draw a FBO with a custom post-process shader
        /// @note Sets the uniform "u_texture" to texture unit if exists
        void draw_fbo(rgl::fbo_t fbo, shader_t shader);

        /// @brief Draw a FBO with regular textured rect
        void draw_fbo(rgl::fbo_t fbo, vec2f_t pos, vec2f_t size);

        /// @brief Draw a rectangle
        /// @param rect Rectangle
        /// @param color Color
        /// @param rotation Rotation in degrees
        /// @param roundedness Roundedness [0-1]
        /// @param lines Draw lines
        void draw_rectangle(rocket::fbounding_box rect, rocket::rgba_color color = { 0, 0, 0, 255 }, float rotation = 0.f, float roundedness = 0.f, bool lines = false);

        /// @brief Draw a rectangle
        /// @param pos Position
        /// @param size Size
        /// @param color Color
        /// @param rotation Rotation in degrees
        /// @param roundedness Roundedness [0-1]
        /// @param lines Draw lines
        void draw_rectangle(rocket::vec2f_t pos, rocket::vec2f_t size, rocket::rgba_color color = { 0, 0, 0, 255 }, float rotation = 0.f, float roundedness = 0.f, bool lines = false);

        /// @brief Draw a circle
        /// @param pos Position
        /// @param radius Radius
        /// @param color Color
        /// @param thickness <=0 if solid, >0 if ring
        void draw_circle(rocket::vec2f_t pos, float radius, rocket::rgba_color color = { 0, 0, 0, 255 }, int thickness = 0);

        /// @brief Draw a polygon
        /// @param pos Position
        /// @param radius Radius
        /// @param color Color
        /// @note Uses [sides] many triangles
        void draw_polygon(rocket::vec2f_t pos, float radius, rocket::rgba_color color = { 0, 0, 0, 255 }, int sides = 3, float rotation = 0.f);

        /// @brief Draw a texture
        /// @param texture Texture
        /// @param rect Rectangle
        /// @param rotation Rotation in degrees
        /// @param roundedness Roundedness [0-1]
        void draw_texture(std::shared_ptr<rocket::texture_t> texture, rocket::fbounding_box rect, float rotation = 0.f, float roundedness = 0.f);

        /// @brief Draw a texture using atlas
        /// @param texture Texture Atlas
        /// @param rect Rectangle
        /// @param texture_position_in_atlas Texture Position
        /// @param texture_size_in_atlas Texture Size
        /// @param rotation Rotation in degrees
        /// @param roundedness Roundedness [0-1]
        void draw_atlas_texture(std::shared_ptr<rocket::texture_t> texture, rocket::fbounding_box rect, rocket::vec2f_t texture_position_in_atlas, rocket::vec2f_t texture_size_in_atlas, float rotation = 0.f, float roundedness = 0.f);

        /// @brief Make a texture ready for drawing
        /// @note Not needed to be called before drawing
        /// @note Pass render_mode_t::texture_filter_none for GL_NEAREST texture filtering
        void make_ready_texture(std::shared_ptr<rocket::texture_t> texture);

        /// @brief Draw text
        /// @note Does text.length drawcalls, use render cache to reduce to 1 drawcall
        /// @param text Text
        /// @param position Position
        void draw_text(const rocket::text_t &text, vec2f_t position);

        /// @brief Draw a singular pixel
        /// @param pos Position
        /// @param color Color
        void draw_pixel(rocket::vec2f_t pos, rocket::rgba_color color);
    public:
        /// @brief Draw FPS at the top left
        void draw_fps(vec2f_t pos = { 10, 10 });
    public:
        /// @brief Set Wireframe State
        void set_wireframe(bool);
        /// @brief Set Vsync
        void set_vsync(bool);
        /// @brief Set FPS
        void set_fps(int fps = 60);
        /// @brief End scissor mode
        void end_scissor_mode();
        /// @brief End render mode
        void end_render_mode(render_mode_t mode);
        /// @brief End frame
        void end_frame();
        /// @brief Check if frame has ended
        bool has_frame_ended();
        /// @brief Set graphics settings
        void set_graphics_settings(graphics_settings_t graphics);
        /// @brief Set viewport size
        void set_viewport_size(vec2f_t size);
        /// @brief Set viewport offset
        /// @param zero_pos The offset (zero position)
        void set_viewport_offset(vec2f_t zero_pos);
        /// @brief Sets the camera
        void set_camera(camera_2d *cam);
        /// @brief Close the renderer2d
        /// @note Does not close the OpenGL Context fully
        void close();
    public:
        /// @brief Get Wireframe State
        bool get_wireframe();
        /// @brief Get Vsync
        bool get_vsync();
        /// @brief Get FPS
        /// @note Gives the TARGET FPS,
        /// @note NOT the current fps
        int get_fps();
        /// @brief Get Delta Time
        double get_delta_time();
        /// @brief Get number of frames elapsed since first frame
        uint64_t get_framecount();
        /// @brief For proper drawcall tracking,
        /// @brief you should probably call this
        /// @brief after end_frame() or just before
        int get_drawcalls();
        /// @brief Gets the draw metrics
        ///         contains Avg, Max, Min: FPS, Frametime
        rgl::draw_metrics_t get_draw_metrics();
        /// @brief Get graphics settings
        const graphics_settings_t &get_graphics_settings();
        /// @brief Get the current framebuffer texture
        /// @note Allocates a new texture and destroys every frame
        /// @note Use render_cache_t::get_texture() to avoid performance
        ///       degradation
        /// @brief Is stored on GPU only 
        /// @brief Lifetime managed automatically
        rgl::scoped_gl_texture_t get_framebuffer_texture();
        /// @brief Get the active camera
        /// @note may return nullptr
        camera_2d *get_camera();
        /// @brief Get the active camera (if any) matrix
        /// @note may return nullptr
        glm::mat4 get_camera_matrix();
    public:
        /// @brief Get Current FPS
        float get_current_fps();
    public:
        /// @brief Create a render cache to draw into
        render_cache_t* create_render_cache(std::function<void(renderer_2d *ren)> draw_cb);
        /// @brief Invalidate the render cache and force a redraw
        void invalidate_render_cache(render_cache_t *c);
        /// @brief Begins rendering to render_cache
        void begin_render_cache(render_cache_t *c);
        /// @brief Ends rendering to render_cache
        void end_render_cache(render_cache_t *c);
        /// @brief Draw contents of render_cache to screen
        void draw_render_cache(render_cache_t *c, rocket::vec2f_t pos, rocket::vec2f_t sz);
        /// @brief Draw contents of render_cache to screen
        void draw_render_cache(render_cache_t *c, rocket::fbounding_box bbox);
        /// @brief Destroy the render cache and free it's resources
        /// @note Any operations on that cache after destruction
        ///       is Undefined Behaviour
        /// @note Is a reference, so modifies your ptr to nullptr
        void destroy_render_cache(render_cache_t *&c);
    public:
        /// @brief Initialize the renderer
        /// @param window Window
        /// @param fps FPS = 60
        renderer_2d(window_backend_i *window, int fps = 60, renderer_flags_t flags = {});
    public:
        ~renderer_2d();
    };

    /// @brief 2D camera
    struct camera_2d {
        /// @brief Zoom multiplier
        float zoom = 1.f;
        /// @brief Offset from 0, 0 (Top left)
        vec2<float> offset;
        /// @brief Rotation
        float rotation = 0.f;

        rocket::vec2f_t world_to_screen(rocket::vec2f_t world);
        rocket::vec2f_t screen_to_world(rocket::vec2f_t screen);
    };
}

#endif
