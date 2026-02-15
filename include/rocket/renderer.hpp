#ifndef ROCKETGE__RENDERER_HPP
#define ROCKETGE__RENDERER_HPP

#include "macros.hpp"
#include <array>
#include <cstdint>
#include "glfnldr.hpp"
#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include "asset.hpp"
#include "rocket/rgl.hpp"
#include "types.hpp"
#include "window.hpp"
#include "shader.hpp"
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace rocket {
    constexpr int fps_uncapped = -1;
    struct instanced_quad_t {
        vec2f_t pos = {0,0};
        vec2f_t size = {0,0};

        GLuint gltxid = 0;
        rgba_color color = {0,0,0,0};
    };
    struct renderer_flags_t {
        bool fxaa_simplified = false;
        bool share_renderer_as_global = true;
        /// @brief Show the splash screen
        bool show_splash = true;
        /// @brief (Advanced) Change Glfnldr backend if available
        /// @note List available backends with glfnldr::get_backends()
        glfnldr::backend_t glfnldr_backend = ROCKETGE__GLFNLDR_BACKEND_ENUM;
    };
    enum class render_mode_t {
        /// @brief Enables a preloaded simplified FXAA Shader
        /// @note HEAVILY BROKEN, rendering with this is just
        ///         all over the place... :(
        fxaa
    };

    struct graphics_settings_t {
        
    };

    struct camera_2d;

    class renderer_2d {
    private:
        window_t *window = nullptr;
        camera_2d *cam = nullptr;
        int fps = 60;
        bool wireframe = false;
        bool vsync = false;

        std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
        double last_time;
        double frame_start_time;
        double delta_time;

        uint64_t frame_counter = 0;

        std::vector<instanced_quad_t> batch;
        bool batched = false;

        bool frame_started = false;

        rocket::vec2f_t override_viewport_size = {-1, -1};
        rocket::vec2f_t override_viewport_offset = {-1, -1};

        std::vector<render_mode_t> active_render_modes;
        
        renderer_flags_t flags;

        bool splash_shown = false;

        graphics_settings_t graphics_settings;

        friend class renderer_3d;
        friend class font_t;
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
        void push_framebuffer(std::vector<rgba_color> &framebuffer);
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
        void draw_shader(shader_t shader);

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

        /// @brief Make a texture ready for drawing
        /// @note Not needed to be called before drawing
        void make_ready_texture(std::shared_ptr<rocket::texture_t> texture);

        /// @brief Draw text
        /// @param text Text
        /// @param position Position
        void draw_text(rocket::text_t &text, vec2f_t position);

        /// @brief Draw a singular pixel
        /// @param pos Position
        /// @param color Color
        void draw_pixel(rocket::vec2f_t pos, rocket::rgba_color color);
    public:
        /// @brief All drawcalls will be redirected to an internal buffer
        /// @brief and be drawn later
        /// @note Unimplemented
        ROCKETGE__NOT_IMPLEMENTED void begin_batch();

        /// @brief All drawcalls that have been stored in the buffer will
        /// @brief be drawn now
        /// @note Unimplemented
        ROCKETGE__NOT_IMPLEMENTED void end_batch(size_t max_batch_size = 2048);
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
        /// @note Unimplemented
        ROCKETGE__NOT_IMPLEMENTED void set_camera(camera_2d *cam);
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
        /// @brief Is stored on GPU only 
        /// @brief Lifetime managed automatically
        rgl::scoped_gl_texture_t get_framebuffer_texture();
        /// @brief Get the active camera
        /// @note may return nullptr
        /// @note Unimplemented
        ROCKETGE__NOT_IMPLEMENTED camera_2d *get_camera();
    public:
        /// @brief Get Current FPS
        int get_current_fps();
    public:
        /// @brief Initialize the renderer
        /// @param window Window
        /// @param fps FPS = 60
        renderer_2d(window_t *window, int fps = 60, renderer_flags_t flags = {});
    public:
        ~renderer_2d();
    };

    /// @brief 2D camera
    struct camera_2d {
        /// @brief Zoom multiplier
        float zoom = 1.f;
        /// @brief Offset from 0, 0 (Top left)
        vec2<float> offset;
    };
}

#endif
