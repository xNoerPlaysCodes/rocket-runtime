#include <GL/glew.h>
#include "../include/renderer.hpp"
#include "util.hpp"
#include <GLFW/glfw3.h>
#include <cmath>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <glm/glm.hpp>                    // core GLM types like vec2, mat4
#include <glm/gtc/matrix_transform.hpp>   // for glm::translate, glm::scale, glm::ortho
#include <glm/gtc/type_ptr.hpp>           // for glm::value_ptr if you prefer using that

#define DEBUG_GL_CHECK_ERROR(x) \
    x; \
    { \
        GLenum err = glGetError(); \
        if (err != GL_NO_ERROR) { \
            std::cerr << "[GL ERROR] " << #x << " -> " << util::format_error("OpenGL error", err, "OpenGL", "warn") << std::endl; \
        } \
    }

namespace rocket {
    std::vector<shader_t> shader_cache;

    renderer_3d::renderer_3d(window_t *window, camera3d_t *cam, int fps) {
        this->window = window;
        this->cam = cam;
        this->fps = fps;

        glfwMakeContextCurrent(window->glfw_window);

        if (!util::glinitialized()) {
            util::glinit(true);

            // Must make sure OpenGL context is current before glewInit
            if (!window || !window->glfw_window) {
                std::cerr << "[ERROR] Invalid window pointer or not initialized!\n";
                std::exit(1);
            }
            glfwMakeContextCurrent(window->glfw_window);

            GLenum err = glewInit();
            if (err != GLEW_OK) {
                const GLubyte* err_str = glewGetErrorString(err);
                std::cerr << util::format_error(reinterpret_cast<const char*>(err_str), err, "glew", "fatal");
                std::exit(1);
            }

            // Clear any errors from glewInit
            while (glGetError() != GL_NO_ERROR) {};

            // Set viewport
            glViewport(0, 0, window->size.x, window->size.y);
            util::gl_setup_perspective({
                (float) window->size.x,
                (float) window->size.y,
                cam->render_distance
            }, cam->fov);

            // Blending (alpha support)
            glEnable(GL_BLEND);
            glEnable(GL_MULTISAMPLE);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // Enable SRGB framebuffer if needed
            glEnable(GL_FRAMEBUFFER_SRGB);

            // 3D Bits
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
        }
    }

    void renderer_3d::begin_frame() {
        start_time = std::chrono::high_resolution_clock::now();
        frame_start_time = glfwGetTime();
        delta_time = frame_start_time - last_time;
        last_time = frame_start_time;
    }
    void renderer_3d::clear(rocket::rgba_color color) {
        glGetError();

        vec4<float> clr = util::glify_a(color);
        glClearColor(clr.x, clr.y, clr.z, clr.w);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    // The ting
    
    void __move_camera_front_back(camera3d_t *cam, float speed, bool use_delta_time, bool forward, double delta_time, bool can_up, bool can_down) {
        float move = speed * (use_delta_time ? (float)delta_time : 1.f);

        // Base direction from yaw and pitch
        glm::vec3 dir = glm::vec3(
            cos(glm::radians(cam->yaw)) * cos(glm::radians(cam->pitch)),
            sin(glm::radians(cam->pitch)),
            sin(glm::radians(cam->yaw)) * cos(glm::radians(cam->pitch))
        );

        // If up/down movement is not allowed, zero out the y component
        if (!can_up && !can_down)
            dir.y = 0.f;
        else if (!can_up && dir.y > 0.f)
            dir.y = 0.f;
        else if (!can_down && dir.y < 0.f)
            dir.y = 0.f;

        dir = glm::normalize(dir);

        glm::vec3 pos = glm::vec3(cam->pos.x, cam->pos.y, cam->pos.z);
        pos += (forward ? dir : -dir) * move;

        cam->pos.x = pos.x;
        cam->pos.y = pos.y;
        cam->pos.z = pos.z;
    }

 
    void __move_camera_left_right(camera3d_t *cam, float speed, bool use_delta_time, bool left, double delta_time_) {
        float move = speed * (use_delta_time ? (float)delta_time_ : 1.f);

        glm::vec3 front = glm::normalize(glm::vec3(
            cos(glm::radians(cam->yaw)) * cos(glm::radians(cam->pitch)),
            sin(glm::radians(cam->pitch)),
            sin(glm::radians(cam->yaw)) * cos(glm::radians(cam->pitch))
        ));

        glm::vec3 up = glm::vec3(0, 1, 0); // world up
        glm::vec3 right = glm::normalize(glm::cross(front, up));

        glm::vec3 pos = glm::vec3(cam->pos.x, cam->pos.y, cam->pos.z);
        pos += (left ? -right : right) * move;

        cam->pos.x = pos.x;
        cam->pos.y = pos.y;
        cam->pos.z = pos.z;
    }

    void renderer_3d::move_cam_forward(float speed, bool use_delta_time, bool can_move_up_using_forward_backward, bool can_move_down_using_forward_backward) {
        __move_camera_front_back(cam, speed, use_delta_time, true, delta_time, can_move_up_using_forward_backward, can_move_down_using_forward_backward);
    }

    void renderer_3d::move_cam_backward(float speed, bool use_delta_time, bool can_move_up_using_forward_backward, bool can_move_down_using_forward_backward) {
        __move_camera_front_back(cam, speed, use_delta_time, false, delta_time, can_move_up_using_forward_backward, can_move_down_using_forward_backward);
    }

    void renderer_3d::move_cam_left(float speed, bool use_delta_time) {
        __move_camera_left_right(cam, speed, use_delta_time, true, delta_time);
    }

    void renderer_3d::move_cam_right(float speed, bool use_delta_time) {
        __move_camera_left_right(cam, speed, use_delta_time, false, delta_time);
    }

    void renderer_3d::mouse_move(float sensitivity) {
        double current_x, current_y;
        glfwGetCursorPos(window->glfw_window, &current_x, &current_y);
        if (!util::is_wayland()) {
            glfwSetCursorPos(window->glfw_window, window->size.x / 2.0, window->size.y / 2.0);
        }

        static bool first_mouse = true;
        static double last_x = window->size.x / 2.0;
        static double last_y = window->size.y / 2.0;

        double xpos, ypos;
        glfwGetCursorPos(window->glfw_window, &xpos, &ypos);

        if (first_mouse) {
            last_x = xpos;
            last_y = ypos;
            first_mouse = false;
        }

        sensitivity = sensitivity / 100; // to normal

        float xoffset = (xpos - last_x) * sensitivity;
        float yoffset = (last_y - ypos) * sensitivity; // reversed y

        last_x = xpos;
        last_y = ypos;

        cam->yaw += xoffset;
        cam->pitch += yoffset;

        // Clamp pitch
        if (cam->pitch > 89.0f) cam->pitch = 89.0f;
        if (cam->pitch < -89.0f) cam->pitch = -89.0f;
    }

    void renderer_3d::draw_camera() {
        static glm::vec3 position;
        static bool posinitialized = false;
        if (!posinitialized) {
            position = glm::vec3(cam->pos.x, cam->pos.y, cam->pos.z);
            posinitialized = true;
        }
        position = glm::vec3(cam->pos.x, cam->pos.y, cam->pos.z);

        glm::vec3 front = glm::vec3(cam->front.x, cam->front.y, cam->front.z);
        glm::vec3 up = glm::vec3(cam->up.x, cam->up.y, cam->up.z);

        // TODO RemoveKeybind
 
        front.x = cos(glm::radians(cam->yaw)) * cos(glm::radians(cam->pitch));
        front.y = sin(glm::radians(cam->pitch));
        front.z = sin(glm::radians(cam->yaw)) * cos(glm::radians(cam->pitch));
        front = glm::normalize(front);
        
        this->projection = glm::perspective<float>(
            glm::radians(cam->fov),
            (float) window->size.x / window->size.y,
            0.1f,
            cam->render_distance
        );

        this->view = glm::lookAt(position, position + front, up);
    }

    void renderer_3d::draw_rectangle(rocket::vec3f_t pos, rocket::vec3f_t size, rocket::rgba_color color, float rotation) {
        const char* vertex_src = R"(
            #version 330 core
        layout(location = 0) in vec3 aPos;

        uniform mat4 u_model;
        uniform mat4 u_view;
        uniform mat4 u_proj;

        void main() {
            gl_Position = u_proj * u_view * u_model * vec4(aPos, 1.0);
        }
        )";

        const char* fragment_src = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec4 u_color;
        void main() {
            FragColor = u_color;
        }
        )";

        static GLuint VAO, VBO, EBO, shader;

        static float cuboid[] = {
            // front face
            -1.0f, -0.5f,  0.5f,
             1.0f, -0.5f,  0.5f,
             1.0f,  0.5f,  0.5f,
            -1.0f,  0.5f,  0.5f,
            // back face
            -1.0f, -0.5f, -0.5f,
             1.0f, -0.5f, -0.5f,
             1.0f,  0.5f, -0.5f,
            -1.0f,  0.5f, -0.5f,
        };

        static unsigned int indices[] = {
            // front
            0, 1, 2, 2, 3, 0,
            // right
            1, 5, 6, 6, 2, 1,
            // back
            7, 6, 5, 5, 4, 7,
            // left
            4, 0, 3, 3, 7, 4,
            // top
            3, 2, 6, 6, 7, 3,
            // bottom
            4, 5, 1, 1, 0, 4
        };

        // --glm--

        // Needs 'view', 'projection', 'position' 
        
        // No camera

        // --glm--

        if (VAO == 0) {
            // --- Setup once ---
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(cuboid), cuboid, GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            // Compile shader
            GLuint vs = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vs, 1, &vertex_src, nullptr);
            glCompileShader(vs);

            GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fs, 1, &fragment_src, nullptr);
            glCompileShader(fs);

            shader = glCreateProgram();
            glAttachShader(shader, vs);
            glAttachShader(shader, fs);
            glLinkProgram(shader);

            glDeleteShader(vs);
            glDeleteShader(fs);
        }

        glUseProgram(shader);

        // Color
        glm::vec4 gl_color(
            color.x / 255.0f,
            color.y / 255.0f,
            color.z / 255.0f,
            color.w / 255.0f
        );
        glUniform4fv(glGetUniformLocation(shader, "u_color"), 1, &gl_color[0]);

        // Model
        glm::mat4 model = glm::mat4(1.0f);

        model = glm::translate(model, glm::vec3(pos.x, pos.y, pos.z));

        model = glm::scale(model, glm::vec3(size.x, size.y, size.z)); // Width, Height, Depth

        glUniformMatrix4fv(glGetUniformLocation(shader, "u_model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shader, "u_view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "u_proj"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }
    void renderer_3d::end_frame() {
        glfwSwapBuffers(this->window->glfw_window);

        if (this->vsync) {
            return; // We're done here
        }

        double frametime_limit = 1.0 / fps;
        // SET_PHYSICS_DELTATIME(delta_time);

        frame_counter++;

        double frame_end_time = glfwGetTime();
        double frame_duration = frame_end_time - frame_start_time;

        auto err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cout << util::format_error(reinterpret_cast<const char *>(glewGetErrorString(err)), err, "OpenGL", "fatal");
            this->window->close();
            std::exit(1);
        }
        glFlush();

        if (frame_duration < frametime_limit) {
            double sleep_time = frametime_limit - frame_duration;
            std::this_thread::sleep_for(std::chrono::duration<double>(sleep_time));
        }
    }

    void renderer_3d::RocketRuntime_Debug_testdraw() {
        
    }

    void renderer_3d::set_fps(int fps) { this->fps = fps; }
    void renderer_3d::set_wireframe(bool wireframe) { this->wireframe = wireframe; }
    void renderer_3d::set_vsync(bool vsync) { this->vsync = vsync; }

    bool renderer_3d::get_wireframe() { return wireframe; }
    bool renderer_3d::get_vsync() { return vsync; }
    int renderer_3d::get_fps() { return fps; }

    void renderer_3d::close() {
        if (window == nullptr) {
            std::exit(0);
        }
        window->close();
        window = nullptr;
    }

    renderer_3d::~renderer_3d() {
        close();
    }
}
