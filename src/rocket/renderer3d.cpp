#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <GL/glew.h>
#include "../../include/rocket/renderer.hpp"
#include "util.hpp"
#include <GLFW/glfw3.h>
#include <cmath>
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
    renderer_3d::renderer_3d(window_t *window, camera3d_t *cam, int fps) : window(window), r2d(window), shellr2d(&r2d) {
        rocket::log("renderer_3d is under construction", "renderer_3d", "construction", "info");
        this->window = window;
        this->cam = cam;
        this->fps = fps;

        glfwMakeContextCurrent(window->glfw_window);

        if (!util::glinitialized()) {
            util::glinit(true);

            // Must make sure OpenGL context is current before glewInit
            if (!window || !window->glfw_window) {
                std::cout << util::format_error("Invalid glfw_window pointer or not initialized", -1, "RocketRuntime", "fatal");
                std::exit(1);
            }

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

            // Blending (alpha support)
            glEnable(GL_BLEND);
            glEnable(GL_MULTISAMPLE);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // Enable SRGB framebuffer if needed
            glEnable(GL_FRAMEBUFFER_SRGB);

            // 3D Bits
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);

            GLuint dummyVAO;
            glGenVertexArrays(1, &dummyVAO);
            glBindVertexArray(dummyVAO);
        }
        util::gl_setup_perspective({
            (float) window->size.x,
            (float) window->size.y,
            cam->render_distance
        }, cam->fov);
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

    struct frustum_plane_t {
        glm::vec3 normal;
        float d;
    };

    std::vector<rocket::vec3f_t> renderer_3d::draw_camera() {
        static glm::vec3 position;
        position = glm::vec3(cam->pos.x, cam->pos.y, cam->pos.z);
        glm::vec3 front;
        front.x = cos(glm::radians(cam->yaw)) * cos(glm::radians(cam->pitch));
        front.y = sin(glm::radians(cam->pitch));
        front.z = sin(glm::radians(cam->yaw)) * cos(glm::radians(cam->pitch));
        front = glm::normalize(front);

        // compute corrected up vector
        glm::vec3 world_up = glm::vec3(0, 1, 0);
        glm::vec3 right = glm::normalize(glm::cross(front, world_up));
        glm::vec3 camera_up = glm::normalize(glm::cross(right, front));

        this->projection = glm::perspective<float>(
            glm::radians(cam->fov),
            (float) window->size.x / window->size.y,
            0.1f,
            cam->render_distance
        );

        this->view = glm::lookAt(position, position + front, camera_up);

        // normal
        glm::mat4 combo = projection * view;

        frustum_plane_t planes[6]; // left, right, bottom, top, near, far

        glm::mat4 m = projection * view;

        // Left
        planes[0].normal.x = m[0][3] + m[0][0];
        planes[0].normal.y = m[1][3] + m[1][0];
        planes[0].normal.z = m[2][3] + m[2][0];
        planes[0].d         = m[3][3] + m[3][0];

        // Right
        planes[1].normal.x = m[0][3] - m[0][0];
        planes[1].normal.y = m[1][3] - m[1][0];
        planes[1].normal.z = m[2][3] - m[2][0];
        planes[1].d         = m[3][3] - m[3][0];

        // And so on...

        // Normalize them
        for (int i = 0; i < 6; i++) {
            float len = glm::length(planes[i].normal);
            planes[i].normal /= len;
            planes[i].d      /= len;
        }

        std::vector<rocket::vec3f_t> frustum;
        frustum.push_back(rocket::vec3f_t{planes[0].normal.x, planes[0].normal.y, planes[0].normal.z});
        frustum.push_back(rocket::vec3f_t{planes[1].normal.x, planes[1].normal.y, planes[1].normal.z});
        frustum.push_back(rocket::vec3f_t{planes[2].normal.x, planes[2].normal.y, planes[2].normal.z});
        frustum.push_back(rocket::vec3f_t{planes[3].normal.x, planes[3].normal.y, planes[3].normal.z});
        frustum.push_back(rocket::vec3f_t{planes[4].normal.x, planes[4].normal.y, planes[4].normal.z});
        frustum.push_back(rocket::vec3f_t{planes[5].normal.x, planes[5].normal.y, planes[5].normal.z});
        return frustum;
    }

    GLuint color_shader = 0;
    GLuint texture_shader = 0;

    GLuint color_VAO = 0, color_VBO = 0, color_EBO = 0;
    GLuint texture_VAO[6], texture_VBO[6], texture_EBO = 0;

    void __compile_shaders() {
        // ---- Color Shader ----
        const char* color_vertex_src = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;

            uniform mat4 u_model;
            uniform mat4 u_view;
            uniform mat4 u_proj;

            void main() {
                gl_Position = u_proj * u_view * u_model * vec4(aPos, 1.0);
            }
        )";

        const char* color_fragment_src = R"(
            #version 330 core
            out vec4 FragColor;
            uniform vec4 u_color;
            void main() {
                FragColor = u_color;
            }
        )";

        float cuboid[] = {
            0.0f, 0.0f, 1.0f,  // Front-bottom-left
            1.0f, 0.0f, 1.0f,  // Front-bottom-right
            1.0f, 1.0f, 1.0f,  // Front-top-right
            0.0f, 1.0f, 1.0f,  // Front-top-left
            0.0f, 0.0f, 0.0f,  // Back-bottom-left
            1.0f, 0.0f, 0.0f,  // Back-bottom-right
            1.0f, 1.0f, 0.0f,  // Back-top-right
            0.0f, 1.0f, 0.0f   // Back-top-left
        };

        unsigned int indices[] = {
            0, 1, 2, 2, 3, 0,
            1, 5, 6, 6, 2, 1,
            7, 6, 5, 5, 4, 7,
            4, 0, 3, 3, 7, 4,
            3, 2, 6, 6, 7, 3,
            4, 5, 1, 1, 0, 4
        };

        glGenVertexArrays(1, &color_VAO);
        glGenBuffers(1, &color_VBO);
        glGenBuffers(1, &color_EBO);

        glBindVertexArray(color_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, color_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cuboid), cuboid, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, color_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &color_vertex_src, nullptr);
        glCompileShader(vs);

        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &color_fragment_src, nullptr);
        glCompileShader(fs);

        color_shader = glCreateProgram();
        glAttachShader(color_shader, vs);
        glAttachShader(color_shader, fs);
        glLinkProgram(color_shader);
        glDeleteShader(vs);
        glDeleteShader(fs);

        // ---- Texture Shader ----
        const char* tex_vertex_src = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec2 aTexCoord;

            out vec2 TexCoord;

            uniform mat4 u_model;
            uniform mat4 u_view;
            uniform mat4 u_proj;

            void main() {
                gl_Position = u_proj * u_view * u_model * vec4(aPos, 1.0);
                TexCoord = aTexCoord;
            }
        )";

        const char* tex_fragment_src = R"(
            #version 330 core
            in vec2 TexCoord;
            out vec4 FragColor;

            uniform sampler2D u_texture;

            void main() {
                FragColor = texture(u_texture, TexCoord);
            }
        )";

        GLuint vs2 = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs2, 1, &tex_vertex_src, nullptr);
        glCompileShader(vs2);

        GLuint fs2 = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs2, 1, &tex_fragment_src, nullptr);
        glCompileShader(fs2);

        texture_shader = glCreateProgram();
        glAttachShader(texture_shader, vs2);
        glAttachShader(texture_shader, fs2);
        glLinkProgram(texture_shader);
        glDeleteShader(vs2);
        glDeleteShader(fs2);
    }
 
    void renderer_3d::draw_rectangle(rocket::fbounding_box_3d fbox, rocket::rgba_color color) {
        rocket::vec3 pos = fbox.pos;
        rocket::vec3 size = fbox.size;
        __compile_shaders();
        glUseProgram(color_shader);

        glm::vec4 gl_color(
            color.x / 255.0f,
            color.y / 255.0f,
            color.z / 255.0f,
            color.w / 255.0f
        );
        glUniform4fv(glGetUniformLocation(color_shader, "u_color"), 1, &gl_color[0]);

        glm::mat4 model = glm::mat4(1.0f);
        // TODO FOR TEXRECT TOO
        model = glm::translate(model, glm::vec3{
            pos.x,
            pos.y - size.y,
            pos.z - size.z,
        });
        model = glm::scale(model, glm::vec3(size.x, size.y, size.z));
        glUniformMatrix4fv(glGetUniformLocation(color_shader, "u_model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(color_shader, "u_view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(color_shader, "u_proj"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(color_VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

    void renderer_3d::end_frame() {
        glfwSwapBuffers(this->window->glfw_window);
        glFlush();

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

    struct textured_vertex_t {
    glm::vec3 pos;
    glm::vec2 uv;
};

    unsigned int indices[6] = { 0, 1, 2, 2, 3, 0 };

    void renderer_3d::draw_texture(rocket::fbounding_box_3d fbox, std::shared_ptr<rocket::texture_t> textures[6]) {
        rocket::vec3f_t pos = fbox.pos;
        rocket::vec3f_t size = fbox.size;
        __compile_shaders();
        struct textured_vertex_t {
            glm::vec3 pos;
            glm::vec2 uv;
        };

        textured_vertex_t vertices[6][4] = {
            // Front face (z = 1)
            { {{0, 0, 1}, {0, 0}}, {{1, 0, 1}, {1, 0}}, {{1, 1, 1}, {1, 1}}, {{0, 1, 1}, {0, 1}} },

            // Right face (x = 1)
            { {{1, 0, 1}, {0, 0}}, {{1, 0, 0}, {1, 0}}, {{1, 1, 0}, {1, 1}}, {{1, 1, 1}, {0, 1}} },

            // Back face (z = 0)
            { {{1, 0, 0}, {0, 0}}, {{0, 0, 0}, {1, 0}}, {{0, 1, 0}, {1, 1}}, {{1, 1, 0}, {0, 1}} },

            // Left face (x = 0)
            { {{0, 0, 0}, {0, 0}}, {{0, 0, 1}, {1, 0}}, {{0, 1, 1}, {1, 1}}, {{0, 1, 0}, {0, 1}} },

            // Top face (y = 1)
            { {{0, 1, 1}, {0, 0}}, {{1, 1, 1}, {1, 0}}, {{1, 1, 0}, {1, 1}}, {{0, 1, 0}, {0, 1}} },

            // Bottom face (y = 0)
            { {{0, 0, 0}, {0, 0}}, {{1, 0, 0}, {1, 0}}, {{1, 0, 1}, {1, 1}}, {{0, 0, 1}, {0, 1}} }
        };

        static GLuint vao[6], vbo[6], ebo;
        static bool initialized = false;

        if (!initialized) {
            glGenBuffers(1, &ebo); // FIRST!

            for (int i = 0; i < 6; i++) {
                glGenVertexArrays(1, &vao[i]);
                glGenBuffers(1, &vbo[i]);

                glBindVertexArray(vao[i]);
                glBindBuffer(GL_ARRAY_BUFFER, vbo[i]);
                glBufferData(GL_ARRAY_BUFFER, sizeof(textured_vertex_t) * 4, vertices[i], GL_STATIC_DRAW);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(textured_vertex_t), (void*)0);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(textured_vertex_t), (void*)(sizeof(glm::vec3)));
                glEnableVertexAttribArray(1);
            }

            initialized = true;
        }

        glUseProgram(texture_shader);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3{
            pos.x,
            pos.y - size.y,
            pos.z - size.z,
        });
        model = glm::scale(model, glm::vec3(size.x, size.y, size.z));

        glUniformMatrix4fv(glGetUniformLocation(texture_shader, "u_model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(texture_shader, "u_view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(texture_shader, "u_proj"), 1, GL_FALSE, glm::value_ptr(projection));

        for (int i = 0; i < 6; i++) {
            if (!textures[i]) continue;

            if (textures[i]->glid == 0) {
                glGenTextures(1, &textures[i]->glid);
                glBindTexture(GL_TEXTURE_2D, textures[i]->glid);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, textures[i]->size.x, textures[i]->size.y, 0,
                             textures[i]->channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, textures[i]->data.data());

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                textures[i]->data.clear();
                textures[i]->data.shrink_to_fit();
            }

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[i]->glid);
            GLint loc = glGetUniformLocation(texture_shader, "u_texture");
            if (loc >= 0) {
                glUniform1i(loc, 0);
            }

            glBindVertexArray(vao[i]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
    }

    void renderer_3d::set_culling(bool c) {
        this->culling = c;
        if (c) {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glFrontFace(GL_CCW);
        } else {
            glDisable(GL_CULL_FACE);
        }
    }

    bool renderer_3d::get_culling() {
        return this->culling;
    }

    void renderer_3d::begin_2d() {
        glDisable(GL_DEPTH_TEST);
    }

    shell_renderer_2d &renderer_3d::get_r2d() {
        return this->shellr2d;
    }

    void shell_renderer_2d::draw_rectangle(rocket::fbounding_box rect, rocket::rgba_color color, float rotation) {
        this->r->draw_rectangle(rect, color, rotation);
    }
    void shell_renderer_2d::draw_circle(rocket::vec2f_t pos, float radius, rocket::rgba_color color) {
        this->r->draw_circle(pos, radius, color);
    }
    void shell_renderer_2d::draw_texture(std::shared_ptr<rocket::texture_t> texture, rocket::fbounding_box rect, float rotation) {
        this->r->draw_texture(texture, rect, rotation);
    }

    void renderer_3d::end_2d() {
        glEnable(GL_DEPTH_TEST);
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
        this->r2d.close();
        window = nullptr;
    }

    renderer_3d::~renderer_3d() {
        close();
    }

    shell_renderer_2d::shell_renderer_2d(renderer_2d *r) {
        this->r = r;
    }
    shell_renderer_2d::~shell_renderer_2d() {}
}
