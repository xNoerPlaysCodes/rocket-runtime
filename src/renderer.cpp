#include <iostream>
#include <GL/glew.h>
#include "../include/rocket/renderer.hpp"
#include "util.hpp"
#include <GLFW/glfw3.h>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/vector_float3.hpp>
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

    GLuint rectVAO, rectVBO;

    renderer_2d::renderer_2d(window_t *window, int fps) {
        this->window = window;
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
            DEBUG_GL_CHECK_ERROR(glViewport(0, 0, window->size.x, window->size.y));

            // Generate and bind VAO/VBO
            DEBUG_GL_CHECK_ERROR(glGenVertexArrays(1, &rectVAO));
            DEBUG_GL_CHECK_ERROR(glGenBuffers(1, &rectVBO));

            DEBUG_GL_CHECK_ERROR(glBindVertexArray(rectVAO));
            DEBUG_GL_CHECK_ERROR(glBindBuffer(GL_ARRAY_BUFFER, rectVBO));

            float square_vertices[] = {
                0.0f, 0.0f,
                1.0f, 0.0f,
                1.0f, 1.0f,
                0.0f, 0.0f,
                1.0f, 1.0f,
                0.0f, 1.0f
            };

            DEBUG_GL_CHECK_ERROR(glBufferData(GL_ARRAY_BUFFER, sizeof(square_vertices), square_vertices, GL_STATIC_DRAW));
            DEBUG_GL_CHECK_ERROR(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0));
            DEBUG_GL_CHECK_ERROR(glEnableVertexAttribArray(0));

            DEBUG_GL_CHECK_ERROR(glBindBuffer(GL_ARRAY_BUFFER, 0));
            DEBUG_GL_CHECK_ERROR(glBindVertexArray(0));

            // Blending (alpha support)
            DEBUG_GL_CHECK_ERROR(glEnable(GL_BLEND));
            DEBUG_GL_CHECK_ERROR(glEnable(GL_MULTISAMPLE));
            DEBUG_GL_CHECK_ERROR(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

            // Enable SRGB framebuffer if needed
            DEBUG_GL_CHECK_ERROR(glEnable(GL_FRAMEBUFFER_SRGB));
        }
        util::gl_setup_ortho(window->size);
    }

    void renderer_2d::draw_circle(rocket::vec2f_t pos, float radius, rocket::rgba_color color) {
        std::cout << util::format_error("Function not implemented", -1, "RocketRuntime", "fatal-to-function");
    }

    void renderer_2d::begin_frame() {
        start_time = std::chrono::high_resolution_clock::now();
        frame_start_time = glfwGetTime();
        delta_time = frame_start_time - last_time;
        last_time = frame_start_time;
    }

    void renderer_2d::clear(rocket::rgba_color color) {
        glGetError();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        vec4<float> clr = util::glify_a(color);
        glClearColor(clr.x, clr.y, clr.z, clr.w);
    }

    void renderer_2d::draw_texture(std::shared_ptr<rocket::texture_t> texture, rocket::fbounding_box rect, float rotation) {
        static GLuint shader_program = 0;

        if (shader_program == 0) {
            const char* vert_src = R"(
                #version 330 core
                layout(location = 0) in vec2 aPos;
                out vec2 v_uv;
                uniform mat4 u_transform;
                void main() {
                    v_uv = aPos;
                    gl_Position = u_transform * vec4(aPos, 0.0, 1.0);
                }
            )";

            const char* frag_src = R"(
                #version 330 core
                in vec2 v_uv;
                out vec4 FragColor;

                uniform sampler2D u_texture;

                void main() {
                    FragColor = texture(u_texture, v_uv);
                }
            )";

            GLuint vs = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vs, 1, &vert_src, nullptr);
            glCompileShader(vs);

            GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fs, 1, &frag_src, nullptr);
            glCompileShader(fs);

            shader_program = glCreateProgram();
            glAttachShader(shader_program, vs);
            glAttachShader(shader_program, fs);
            glLinkProgram(shader_program);

            glDeleteShader(vs);
            glDeleteShader(fs);
        }

        glUseProgram(shader_program);

        if (texture->glid == 0) {
            glGenTextures(1, &texture->glid);
            glBindTexture(GL_TEXTURE_2D, texture->glid);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, texture->size.x, texture->size.y, 0,
                         texture->channels == 4 ? GL_RGBA : GL_RGB,
                         GL_UNSIGNED_BYTE, texture->data.data());

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // Optional: Clear texture data to save memory
            texture->data.clear();
            texture->data.shrink_to_fit();
        }

        // Build transform
        glm::mat4 projection = glm::ortho(
            0.0f, static_cast<float>(window->size.x),
            static_cast<float>(window->size.y), 0.0f,
            -1.0f, 1.0f
        );

        float cx = rect.pos.x + rect.size.x * 0.5f;
        float cy = rect.pos.y + rect.size.y * 0.5f;

        glm::mat4 transform = projection
            * glm::translate(glm::mat4(1.0f), glm::vec3(cx, cy, 0.0f))
            * glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f))
            * glm::translate(glm::mat4(1.0f), glm::vec3(-rect.size.x * 0.5f, -rect.size.y * 0.5f, 0.0f))
            * glm::scale(glm::mat4(1.0f), glm::vec3(rect.size.x, rect.size.y, 1.0f));

        
        // Set transform matrix only
        glUniformMatrix4fv(glGetUniformLocation(shader_program, "u_transform"), 1, GL_FALSE, glm::value_ptr(transform));

        // Bind texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture->glid);
        glUniform1i(glGetUniformLocation(shader_program, "u_texture"), 0); // still required

        // Draw
        glBindVertexArray(rectVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }
 
    void renderer_2d::draw_rectangle(rocket::fbounding_box rect, rocket::rgba_color color, float rotation) {
        static GLuint shader_program = 0;

        rotation = glm::radians(rotation);

        if (shader_program == 0) {
            // Vertex shader source
            const char *vert_src = R"(
                #version 330 core
                layout(location = 0) in vec2 aPos;
                uniform mat4 u_transform;
                void main() {
                    gl_Position = u_transform * vec4(aPos, 0.0, 1.0);
                }
            )";

            // Fragment shader source
            const char *frag_src = R"(
                #version 330 core
                out vec4 FragColor;
                uniform vec4 u_color;
                void main() {
                    FragColor = u_color;
                }
            )";



            // Compile vertex shader
            GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vert_shader, 1, &vert_src, nullptr);
            glCompileShader(vert_shader);

            // Compile fragment shader
            GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(frag_shader, 1, &frag_src, nullptr);
            glCompileShader(frag_shader);

            // Link shader program
            shader_program = glCreateProgram();
            glAttachShader(shader_program, vert_shader);
            glAttachShader(shader_program, frag_shader);
            glLinkProgram(shader_program);

            // Check for linking errors
            GLint success;
            glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
            if (!success) {
                char infoLog[512];
                glGetProgramInfoLog(shader_program, 512, nullptr, infoLog);
                std::cout << util::format_error(infoLog, -1, "shader linker", "fatal-to-function");
            }

            // Cleanup
            glDeleteShader(vert_shader);
            glDeleteShader(frag_shader);
        }

        glUseProgram(shader_program);

        
        glm::mat4 projection = glm::ortho(
            0.0f, static_cast<float>(window->size.x),
            static_cast<float>(window->size.y), 0.0f,
            -1.0f, 1.0f
        );

        float center_x = rect.pos.x + rect.size.x * 0.5f;
        float center_y = rect.pos.y + rect.size.y * 0.5f;

        glm::mat4 transform = projection
            * glm::translate(glm::mat4(1.0f), glm::vec3(center_x, center_y, 0.0f))
            * glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0.0f, 0.0f, 1.0f))
            * glm::translate(glm::mat4(1.0f), glm::vec3(-rect.size.x * 0.5f, -rect.size.y * 0.5f, 0.0f))
            * glm::scale(glm::mat4(1.0f), glm::vec3(rect.size.x, rect.size.y, 1.0f));

        // Set uniforms safely
        GLint u_transform = glGetUniformLocation(shader_program, "u_transform");
        if (u_transform != -1) {
            glUniformMatrix4fv(u_transform, 1, GL_FALSE, glm::value_ptr(transform));
        }

        GLint u_color = glGetUniformLocation(shader_program, "u_color");
        if (u_color != -1) {
            glUniform4f(u_color,
                color.x / 255.0f,
                color.y / 255.0f,
                color.z / 255.0f,
                color.w / 255.0f);
        }

        // Draw
        glBindVertexArray(rectVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    } 

    void renderer_2d::set_wireframe(bool x) {
        this->wireframe = x;
        glPolygonMode(GL_FRONT_AND_BACK, x ? GL_LINE : GL_FILL);
    }

    void renderer_2d::set_vsync(bool x) {
        this->vsync = x;
        glfwSwapInterval(x ? 1 : 0);
    }

    void renderer_2d::set_fps(int fps) {
        this->fps = fps;
    }

    void renderer_2d::end_frame() {
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

    double renderer_2d::get_delta_time() {
        return delta_time;
    }

    int renderer_2d::get_fps() {
        return fps;
    }

    bool renderer_2d::get_vsync() {
        return vsync;
    }

    bool renderer_2d::get_wireframe() {
        return wireframe;
    }

    void renderer_2d::close() {
        if (window == nullptr) {
            std::cout << util::format_error("window already closed", -1, "RocketRuntime", "warn");
            return;
        }
        window->close();
    }

    renderer_2d::~renderer_2d() {
        close();
    }
}
