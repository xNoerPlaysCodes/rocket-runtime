#pragma once

#include <rocket/shader.hpp>
#include <rocket/types.hpp>
#include <functional>
#include <rocket/window_helpers.hpp>
#include "rocket/asset.hpp"
#include <rocket/glfnldr.hpp>
#include <string>
#include <rocket/io.hpp>
#include <rocket/renderer_helpers.hpp>
#include <glm/mat4x2.hpp>

namespace rgl {
    std::vector<std::string> init_gl(rocket::vec2f_t viewport_size, glfnldr::backend_t bkend);
}

namespace rocket {
    struct window_backend_i;
    struct renderer_2d_impl_t;
    struct camera_2d;
    class renderer_2d_i {
    protected:
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

        friend window_backend_i* __r2d_get_window(rocket::renderer_2d_i*);
        friend class renderer_3d;
        friend class font_t;
        friend class asset_manager_t;
    protected:
        enum class gfx_chk_result {
            not_drawable,
            drawable,
        };
        virtual gfx_chk_result check_graphics_settings(rocket::vec2f_t pos, rocket::vec2f_t sz) = 0;
    private:
        virtual api_object_t upload_font_texture_to_gpu(rocket::vec2i_t size, const std::vector<uint8_t> &bitmap) = 0;
        virtual void clean_gpu_resource(api_object_t object) = 0;
    public:
        /// @brief Check if frame has begun
        virtual bool has_frame_began() = 0;
        /// @brief Begin frame
        virtual void begin_frame() = 0;
        /// @brief Show the splash ignoring flags
        virtual void show_splash() = 0;
        /// @brief Begin render mode
        virtual void begin_render_mode(render_mode_t) = 0;
        /// @brief Get a contiguous block of pixels
        /// @brief adjusted to viewport size
        /// @brief VERY SLOW use only for SOFTWARE rendering
        virtual std::vector<rgba_color> get_framebuffer() = 0;
        /// @brief Push a contiguous block of pixels
        /// @brief adjusted to viewport size
        /// @brief ONLY for SOFTWARE rendering
        virtual void push_framebuffer(const std::vector<rgba_color> &framebuffer) = 0;
        /// @brief Get the size of the viewport
        virtual vec2f_t get_viewport_size() = 0;
        /// @brief Begin scissor mode
        virtual void begin_scissor_mode(rocket::fbounding_box rect) = 0;
        /// @brief Begin scissor mode
        virtual void begin_scissor_mode(rocket::vec2f_t pos, rocket::vec2f_t size) = 0;
        /// @brief Begin scissor mode
        virtual void begin_scissor_mode(float x, float y, float sx, float sy) = 0;
        /// @brief Clear the screen
        virtual void clear(rocket::rgba_color color = { 255, 255, 255, 255 }) = 0;

        /// @brief Draw a shader
        /// @note Uniforms to be pre-set in the shader by shader.set_uniform(...)
        virtual void draw_shader(shader_t shader) = 0;

        /// @brief Draw a rectangle
        /// @param rect Rectangle
        /// @param color Color
        /// @param rotation Rotation in degrees
        /// @param roundedness Roundedness [0-1]
        /// @param lines Draw lines
        virtual void draw_rectangle(rocket::fbounding_box rect, rocket::rgba_color color = { 0, 0, 0, 255 }, float rotation = 0.f, float roundedness = 0.f, bool lines = false) = 0;

        /// @brief Draw a rectangle
        /// @param pos Position
        /// @param size Size
        /// @param color Color
        /// @param rotation Rotation in degrees
        /// @param roundedness Roundedness [0-1]
        /// @param lines Draw lines
        virtual void draw_rectangle(rocket::vec2f_t pos, rocket::vec2f_t size, rocket::rgba_color color = { 0, 0, 0, 255 }, float rotation = 0.f, float roundedness = 0.f, bool lines = false) = 0;

        /// @brief Draw a circle
        /// @param pos Position
        /// @param radius Radius
        /// @param color Color
        /// @param thickness <=0 if solid, >0 if ring
        virtual void draw_circle(rocket::vec2f_t pos, float radius, rocket::rgba_color color = { 0, 0, 0, 255 }, int thickness = 0) = 0;

        /// @brief Draw a polygon
        /// @param pos Position
        /// @param radius Radius
        /// @param color Color
        /// @note Uses [sides] many triangles
        virtual void draw_polygon(rocket::vec2f_t pos, float radius, rocket::rgba_color color = { 0, 0, 0, 255 }, int sides = 3, float rotation = 0.f) = 0;

        /// @brief Draw a texture
        /// @param texture Texture
        /// @param rect Rectangle
        /// @param rotation Rotation in degrees
        /// @param roundedness Roundedness [0-1]
        virtual void draw_texture(std::shared_ptr<rocket::texture_t> texture, rocket::fbounding_box rect, float rotation = 0.f, float roundedness = 0.f) = 0;

        /// @brief Draw a texture using atlas
        /// @param texture Texture Atlas
        /// @param rect Rectangle
        /// @param texture_position_in_atlas Texture Position
        /// @param texture_size_in_atlas Texture Size
        /// @param rotation Rotation in degrees
        /// @param roundedness Roundedness [0-1]
        virtual void draw_atlas_texture(std::shared_ptr<rocket::texture_t> texture, rocket::fbounding_box rect, rocket::vec2f_t texture_position_in_atlas, rocket::vec2f_t texture_size_in_atlas, float rotation = 0.f, float roundedness = 0.f) = 0;

        /// @brief Make a texture ready for drawing
        /// @note Not needed to be called before drawing
        /// @note Pass render_mode_t::texture_filter_none for GL_NEAREST texture filtering
        virtual void make_ready_texture(std::shared_ptr<rocket::texture_t> texture) = 0;

        /// @brief Draw text
        /// @note Does text.length drawcalls, use render cache to reduce to 1 drawcall
        /// @param text Text
        /// @param position Position
        virtual void draw_text(const rocket::text_t &text, vec2f_t position) = 0;

        /// @brief Draw a singular pixel
        /// @param pos Position
        /// @param color Color
        virtual void draw_pixel(rocket::vec2f_t pos, rocket::rgba_color color) = 0;
    public:
        /// @brief Draw FPS at the top left
        virtual void draw_fps(vec2f_t pos = { 10, 10 }) = 0;
    public:
        /// @brief Set Wireframe State
        virtual void set_wireframe(bool) = 0;
        /// @brief Set Vsync
        virtual void set_vsync(bool) = 0;
        /// @brief Set FPS
        virtual void set_fps(int fps = 60) = 0;
        /// @brief End scissor mode
        virtual void end_scissor_mode() = 0;
        /// @brief End render mode
        virtual void end_render_mode(render_mode_t mode) = 0;
        /// @brief End frame
        virtual void end_frame() = 0;
        /// @brief Check if frame has ended
        virtual bool has_frame_ended() = 0;
        /// @brief Set graphics settings
        virtual void set_graphics_settings(graphics_settings_t graphics) = 0;
        /// @brief Set viewport size
        virtual void set_viewport_size(vec2f_t size) = 0;
        /// @brief Set viewport offset
        /// @param zero_pos The offset (zero position)
        virtual void set_viewport_offset(vec2f_t zero_pos) = 0;
        /// @brief Sets the camera
        virtual void set_camera(camera_2d *cam) = 0;
        /// @brief Close the renderer2d
        /// @note Does not close the OpenGL Context fully
        virtual void close() = 0;
    public:
        /// @brief Get Wireframe State
        virtual bool get_wireframe() = 0;
        /// @brief Get Vsync
        virtual bool get_vsync() = 0;
        /// @brief Get FPS
        /// @note Gives the TARGET FPS,
        /// @note NOT the current fps
        virtual int get_fps() = 0;
        /// @brief Get Delta Time
        virtual double get_delta_time() = 0;
        /// @brief Get number of frames elapsed since first frame
        virtual uint64_t get_framecount() = 0;
        /// @brief For proper drawcall tracking,
        /// @brief you should probably call this
        /// @brief after end_frame() or just before
        virtual int get_drawcalls() = 0;
        /// @brief Gets the draw metrics
        ///         contains Avg, Max, Min: FPS, Frametime
        virtual rgl::draw_metrics_t get_draw_metrics() = 0;
        /// @brief Get graphics settings
        virtual const graphics_settings_t &get_graphics_settings() = 0;
        /// @brief Get the current framebuffer texture
        /// @note Allocates a new texture and destroys every frame
        /// @note Use render_cache_t::get_texture() to avoid performance
        ///       degradation
        /// @brief Is stored on GPU only 
        /// @brief Lifetime managed automatically
        virtual api_object_t get_framebuffer_texture() = 0;
        /// @brief Get the active camera
        /// @note may return nullptr
        virtual camera_2d *get_camera() = 0;
        /// @brief Get the active camera (if any) matrix
        virtual glm::mat4 get_camera_matrix() = 0;
    public:
        /// @brief Get Current FPS
        virtual float get_current_fps() = 0;
    public:
        /// @brief Create a render cache to draw into
        virtual render_cache_t* create_render_cache(std::function<void(renderer_2d_i *ren)> draw_cb) = 0;
        /// @brief Invalidate the render cache and force a redraw
        virtual void invalidate_render_cache(render_cache_t *c) = 0;
        /// @brief Begins rendering to render_cache
        virtual void begin_render_cache(render_cache_t *c) = 0;
        /// @brief Ends rendering to render_cache
        virtual void end_render_cache(render_cache_t *c) = 0;
        /// @brief Draw contents of render_cache to screen
        virtual void draw_render_cache(render_cache_t *c, rocket::vec2f_t pos, rocket::vec2f_t sz) = 0;
        /// @brief Draw contents of render_cache to screen
        virtual void draw_render_cache(render_cache_t *c, rocket::fbounding_box bbox) = 0;
        /// @brief Destroy the render cache and free it's resources
        /// @note Any operations on that cache after destruction
        ///       is Undefined Behaviour
        /// @note Is a reference, so modifies your ptr to nullptr
        virtual void destroy_render_cache(render_cache_t *&c) = 0;
    public:
        // /// @brief Initialize the renderer
        // /// @param window Window
        // /// @param fps FPS = 60
        // renderer_2d_i(window_backend_i *window, int fps = 60, renderer_flags_t flags = {});
    public:
        virtual ~renderer_2d_i() = default;
    };
}
