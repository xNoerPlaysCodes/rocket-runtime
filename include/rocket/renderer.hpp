#ifndef ROCKETGE__RENDERER_HPP
#define ROCKETGE__RENDERER_HPP

#include "macros.hpp"
#include <array>
#include <cstdint>
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
#include <string>
#include <vector>

#define RGE_FPS_UNCAPPED -1

namespace rocket {
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
    };
    enum class render_mode_t {
        /// @brief Enables a preloaded simplified FXAA Shader
        /// @note HEAVILY BROKEN, rendering with this is just ...
        ///         all over the place... :(
        fxaa
    };
    class renderer_2d {
    private:
        window_t *window;
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
        std::vector<rgba_color> get_framebuffer();
        /// @brief Push a contiguous block of pixels
        /// @brief adjusted to viewport size
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
        void draw_circle(rocket::vec2f_t pos, float radius, rocket::rgba_color color = { 0, 0, 0, 255 });

        /// @brief Draw a polygon
        /// @param pos Position
        /// @param radius Radius
        /// @param color Color
        /// @note Uses [sides] many triangles
        void draw_polygon(rocket::vec2f_t pos, float radius, rocket::rgba_color color = { 0, 0, 0, 255 }, int sides = 3, float rotation = 0.f);

        /// @brief Draw a line
        /// @param p1 Point 1
        /// @param p2 Point 2
        /// @param color Color
        /// @param thickness Thickness
        void draw_line(rocket::vec2f_t p1, rocket::vec2f_t p2, rocket::rgba_color color = { 0, 0, 0, 255 }, float thickness = 1.f);

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
        ROCKETGE__NOT_IMPLEMENTED void begin_batch();

        /// @brief All drawcalls that have been stored in the buffer will
        /// @brief be drawn now
        /// @note Unsupported
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
        /// @brief Set viewport size
        void set_viewport_size(vec2f_t size);
        /// @brief Set viewport offset
        /// @param zero_pos The offset (zero position)
        void set_viewport_offset(vec2f_t zero_pos);
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

    struct camera3d_t {
        /// @brief Field of view
        /// @note Capped to 1* -> 179*
        /// @note In degrees (*)
        float fov = 45.f;
        /// @brief How many units into the distance can be seen
        float render_distance = 100.f;
        /// @brief Camera's position
        vec3f_t pos = { 0.f, 0.f, 0.f };
        /// @brief Don't modify this unless you know what this is
        /// @note Up for the camera? Usually Y-axis
        vec3f_t up = { 0.f, 1.f, 0.f };
        /// @brief Don't modify this unless you know what this is
        vec3f_t lookat = { 0.f, 0.f, 0.f };
        /// @brief yaw
        float yaw = -90.f;
        /// @brief pitch
        float pitch = 0.f;
        /// @brief front, don't modify
        vec3f_t front = { 0, 0, -1 };
    };

    class shell_renderer_2d {
    private:
        renderer_2d *r;
    public:
        void draw_rectangle(rocket::fbounding_box rect, rocket::rgba_color color = { 0, 0, 0, 255 }, float rotation = 0.f);
        void draw_circle(rocket::vec2f_t pos, float radius, rocket::rgba_color color = { 0, 0, 0, 255 });
        void draw_texture(std::shared_ptr<rocket::texture_t> texture, rocket::fbounding_box rect, float rotation = 0.f);
    public:
        shell_renderer_2d(renderer_2d *r);
        ~shell_renderer_2d();
    };

    class ROCKETGE__NOT_IMPLEMENTED renderer_3d {
    private:
        window_t *window;
        int fps;
        bool wireframe;
        bool vsync;

        glm::mat4 projection;
        glm::mat4 view;

        std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
        double last_time;
        double frame_start_time;
        double delta_time;

        uint64_t frame_counter;

        camera3d_t *cam;

        renderer_2d r2d;
        shell_renderer_2d shellr2d;

        bool culling = false;
    private:
        void glinit();
    public:
        void begin_frame();
        void clear(rocket::rgba_color color = { 255, 255, 255, 255 });
        /// @brief Returns the camera frustum's normal
        std::vector<vec3f_t> draw_camera();
        void draw_rectangle(rocket::fbounding_box_3d fbox, rocket::rgba_color color = { 0, 0, 0, 255 });
        void draw_texture(rocket::fbounding_box_3d fbox, std::shared_ptr<rocket::texture_t> textures[6]);

        /// @brief Call AFTER doing ALL your 3D work
        void begin_2d();

        shell_renderer_2d &get_r2d();
        
        /// @brief Call BEFORE ending frame
        void end_2d();

        void end_frame();
    public:
        void set_fps(int);
        void set_wireframe(bool);
        void set_vsync(bool);
        void set_culling(bool);
    public:
        bool get_wireframe();
        bool get_vsync();
        bool get_culling();
        int get_fps();
    public:
        /// @brief Side-to-side utility moving
        void move_cam_left(float speed, bool use_delta_time = true);

        /// @brief Side-to-side utility moving
        void move_cam_right(float speed, bool use_delta_time = true);

        /// @brief Forward utility moving
        void move_cam_forward(float speed, bool use_delta_time = true, bool can_move_up_using_forward_backward = true, bool can_move_down_using_forward_backward = true);

        /// @brief Backward utility moving
        void move_cam_backward(float speed, bool use_delta_time = true, bool can_move_up_using_forward_backward = true, bool can_move_down_using_forward_backward = true);

        /// @brief Mouse
        /// @param sensitivity Percentage
        /// @note Sensitivity is clamped from 0 to 100+ (for extra-high)
        void mouse_move(float sensitivity = 0.1f);
    public:
        renderer_3d(window_t *window, camera3d_t *cam, int fps = 60);
    public:
        void close();
        ~renderer_3d();
    };
}

#endif
