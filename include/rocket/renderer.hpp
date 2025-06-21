#ifndef ROCKETGE__RENDERER_HPP
#define ROCKETGE__RENDERER_HPP

#include "asset.hpp"
#include "types.hpp"
#include "window.hpp"
#include "runtime.hpp"
#include <chrono>
#include <glm/ext/matrix_float4x4.hpp>
#include <string>
#include <vector>

namespace rocket {
    enum class shader_type {
        vertex,
        fragment,
        geometry,
        tessellation_control,
        tesselation_compute,
#ifdef ROCKETGE_OpenGL_SupportComputeShader
        compute
#endif
    };

    struct shader_t {
    private:
        GLuint glshader = 0;
        GLuint glprogram = 0;
        friend class renderer_2d;
    public:
        shader_type type;
        std::string code;

        bool operator==(const shader_t &other) const {
            return type == other.type && code == other.code;
        }
    };

    class renderer_2d {
    private:
        window_t *window;
        int fps;
        bool wireframe;
        bool vsync;

        std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
        double last_time;
        double frame_start_time;
        double delta_time;

        uint64_t frame_counter;
    private:
        void glinit();
    public:
        void begin_frame();
        void clear(rocket::rgba_color color = { 255, 255, 255, 255 });

        void draw_shader(shader_t &shader);

        void draw_rectangle(rocket::fbounding_box rect, rocket::rgba_color color = { 0, 0, 0, 255 }, float rotation = 0.f);
        void draw_circle(rocket::vec2f_t pos, float radius, rocket::rgba_color color = { 0, 0, 0, 255 });

        void draw_texture(std::shared_ptr<rocket::texture_t> texture, rocket::fbounding_box rect, float rotation = 0.f);

        void set_wireframe(bool);
        void set_vsync(bool);
        void set_fps(int fps = 60);
        void end_frame();
        void close();
    public:
        bool get_wireframe();
        bool get_vsync();
        int get_fps();
        double get_delta_time();
    public:
        renderer_2d(window_t *window, int fps = 60);
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

    class renderer_3d {
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
    private:
        void glinit();
    public:
        void begin_frame();
        void clear(rocket::rgba_color color = { 255, 255, 255, 255 });
        void draw_camera();
        void draw_rectangle(rocket::fbounding_box_3d fbox, rocket::rgba_color color = { 0, 0, 0, 255 });
        void draw_texture(rocket::fbounding_box_3d fbox, std::shared_ptr<rocket::texture_t> textures[6]);
        void end_frame();
    public:
        void set_fps(int);
        void set_wireframe(bool);
        void set_vsync(bool);
    public:
        bool get_wireframe();
        bool get_vsync();
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
