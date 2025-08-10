#include <cmath>
#define STB_TRUETYPE_IMPLEMENTATION
#include <iostream>
#include <GL/glew.h>
#include "../../include/rocket/renderer.hpp"
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
    GLuint txVAO, txVBO;
    GLuint textVAO, textVBO;

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

            glGenVertexArrays(1, &txVAO);
            glGenBuffers(1, &txVBO);

            glGenVertexArrays(1, &textVAO);
            glGenBuffers(1, &textVBO);

            DEBUG_GL_CHECK_ERROR(glBindVertexArray(rectVAO));
            DEBUG_GL_CHECK_ERROR(glBindBuffer(GL_ARRAY_BUFFER, rectVBO));

            glBindVertexArray(txVAO);
            glBindBuffer(GL_ARRAY_BUFFER, txVBO);

            glBindVertexArray(textVAO);
            glBindBuffer(GL_ARRAY_BUFFER, textVBO);

            float square_vertices[] = {
                0.0f, 0.0f,
                1.0f, 0.0f,
                1.0f, 1.0f,
                0.0f, 0.0f,
                1.0f, 1.0f,
                0.0f, 1.0f
            };

            glBindVertexArray(rectVAO);
            glBindBuffer(GL_ARRAY_BUFFER, rectVBO);

            glBufferData(GL_ARRAY_BUFFER, sizeof(square_vertices), square_vertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            glBindVertexArray(txVAO);
            glBindBuffer(GL_ARRAY_BUFFER, txVBO);

            glBufferData(GL_ARRAY_BUFFER, sizeof(square_vertices), square_vertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);


            glGenVertexArrays(1, &textVAO);
            glGenBuffers(1, &textVBO);

            glBindVertexArray(textVAO);
            glBindBuffer(GL_ARRAY_BUFFER, textVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0); // aPos

            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
            glEnableVertexAttribArray(1); // aTex

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            // Blending (alpha support)
            DEBUG_GL_CHECK_ERROR(glEnable(GL_BLEND));
            bool gl_blend = true;
            DEBUG_GL_CHECK_ERROR(glEnable(GL_MULTISAMPLE));
            bool gl_multisample = true;
            DEBUG_GL_CHECK_ERROR(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
            bool gl_blendfunc = "GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA";

            // Enable SRGB framebuffer if needed
            DEBUG_GL_CHECK_ERROR(glEnable(GL_FRAMEBUFFER_SRGB));
            bool gl_srgb = true;
            int max_tx_size;
            glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tx_size);
            rocket::log("OpenGL Initialized", "renderer_2d", "constructor", "info");
            const std::vector<std::string> log_messages = {
                "RocketGE Modules:",
#ifdef ROCKETGE__BUILD_QUARK
                "- Quark: [TRUE]",
#else
                "- Quark: [FALSE]",
#endif
#ifdef ROCKETGE__BUILD_ASTRO
                "- AstroUI: [TRUE]",
#else
                "- AstroUI: [FALSE]",
#endif
            "GL Info:",
                "- GL Version: " + std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION))),
                "- GL Vendor: " + std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR))),
                "- GL Activated Functions: ",
                "   - GL_BLEND: [" + (gl_blend ? std::string("TRUE") : std::string("FALSE")) + "]",
                "   - GL_MULTISAMPLE: [" + (gl_multisample ? std::string("TRUE") : std::string("FALSE")) + "]",
                "   - GL_BLEND_FUNC: [" + (gl_blendfunc ? std::string("TRUE") : std::string("FALSE")) + "]",
                "   - GL_FRAMEBUFFER_SRGB: [" + (gl_srgb ? std::string("TRUE") : std::string("FALSE")) + "]",
                "- GL VAO/VBO Pairs Created:",
                "   - rectVAO/VBO: [" + (rectVAO != 0 && rectVBO != 0 ? std::string("TRUE") : std::string("FALSE")) + "]",
                "   - txVAO/VBO: [" + (txVAO != 0 && txVBO != 0 ? std::string("TRUE") : std::string("FALSE")) + "]",
                "   - textVAO/VBO: [" + (textVAO != 0 && textVBO != 0 ? std::string("TRUE") : std::string("FALSE")) + "]",
                "- GL GPU-Specific Values:",
                "   - GL_MAX_TEXTURE_SIZE: " + std::to_string(max_tx_size) + " x " + std::to_string(max_tx_size),
            "Screen Info:",
                "- Screen Size: " + std::to_string(window->size.x) + " x " + std::to_string(window->size.y)
            };

            for (auto &l : log_messages) {
                rocket::log(l, "renderer_2d", "constructor", "info");
            }
        }
        util::gl_setup_ortho(window->size);
    }

    void renderer_2d::draw_circle(rocket::vec2f_t pos, float radius, rocket::rgba_color color) {
        draw_rectangle({ pos, { radius * 2, radius * 2 } }, color, 0, 1);
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
                    vec2 pos = aPos; // 0→1 quad
                    gl_Position = u_transform * vec4(pos, 0.0, 1.0);
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

            texture->data.clear();
            texture->data.shrink_to_fit();
        }

        // Create transform (from 0-1 quad to pixel-space rectangle)
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

        glUniformMatrix4fv(glGetUniformLocation(shader_program, "u_transform"), 1, GL_FALSE, glm::value_ptr(transform));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture->glid);
        glUniform1i(glGetUniformLocation(shader_program, "u_texture"), 0);

        glBindVertexArray(txVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
  
    void renderer_2d::draw_rectangle(rocket::fbounding_box rect, rocket::rgba_color color, float rotation, float roundedness) {
        static GLuint shader_program = 0;

        if (shader_program == 0) {
            const char* vert_src = R"(

#version 330 core
    layout(location = 0) in vec2 aPos; // 0→1 quad coords
    uniform mat4 u_transform;
    out vec2 v_local;

    void main() {
        v_local = aPos; // normalized quad coordinates
        gl_Position = u_transform * vec4(aPos, 0.0, 1.0);
    }

            )";

            const char* frag_src = R"(


#version 330 core
    in vec2 v_local;
    out vec4 FragColor;

    uniform vec4 u_color;   // RGBA 0–1
    uniform float u_radius; // fraction 0..1 of min size
    uniform vec2 u_size;    // rect size in pixels

    void main() {
        vec2 local_px = v_local * u_size;

        // Convert fraction to pixel radius
        float radius_px = u_radius * 0.5 * min(u_size.x, u_size.y);

        // Distance from nearest edge
        vec2 cornerDist = min(local_px, u_size - local_px);

        // Distance from corner arc
        float dist = length(cornerDist - vec2(radius_px));

        // Anti-aliased alpha edge
        float edge_thickness = 1.0; // in pixels
        float alpha = 1.0;

        if (cornerDist.x < radius_px && cornerDist.y < radius_px) {
            alpha = 1.0 - smoothstep(radius_px - edge_thickness, radius_px, dist);
        }

        FragColor = vec4(u_color.rgb, u_color.a * alpha);
    }

            )";

            GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vert_shader, 1, &vert_src, nullptr);
            glCompileShader(vert_shader);

            GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(frag_shader, 1, &frag_src, nullptr);
            glCompileShader(frag_shader);

            shader_program = glCreateProgram();
            glAttachShader(shader_program, vert_shader);
            glAttachShader(shader_program, frag_shader);
            glLinkProgram(shader_program);

            glDeleteShader(vert_shader);
            glDeleteShader(frag_shader);
        }

        glUseProgram(shader_program);

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

        glUniformMatrix4fv(glGetUniformLocation(shader_program, "u_transform"), 1, GL_FALSE, glm::value_ptr(transform));
        glUniform4f(glGetUniformLocation(shader_program, "u_color"),
            color.x / 255.0f,
            color.y / 255.0f,
            color.z / 255.0f,
            color.w / 255.0f);

        // Pass size and radius to shader
        glUniform2f(glGetUniformLocation(shader_program, "u_size"), rect.size.x, rect.size.y);
        glUniform1f(glGetUniformLocation(shader_program, "u_radius"), roundedness);

        glBindVertexArray(rectVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
        
    void renderer_2d::draw_text(rocket::text_t &text, rocket::vec2f_t position) {
        static GLuint shader_program = 0;

        if (shader_program == 0) {
            const char* vert_src = R"(
                #version 330 core
                layout(location = 0) in vec2 aPos;
                layout(location = 1) in vec2 aTex;

                out vec2 TexCoord;

                void main() {
                    gl_Position = vec4(aPos.xy, 0.0, 1.0);
                    TexCoord = aTex;
                }
            )";

            const char* frag_src = R"(
                #version 330 core
                in vec2 TexCoord;
                out vec4 FragColor;

                uniform sampler2D u_texture;
                uniform vec3 u_color;

                void main() {
                    float alpha = texture(u_texture, TexCoord).r;

                    // Gamma correct to linear
                    alpha = pow(alpha, 2.2);

                    // Sharpen edges (smaller = sharper, 1.0 = no change)
                    alpha = pow(alpha, 0.5);

                    // Convert back to sRGB
                    alpha = pow(alpha, 1.0 / 2.2);

                    // Premultiply color
                    vec3 rgb = u_color * alpha;
                    FragColor = vec4(rgb, alpha);
                }
            )";

            // Compile vertex shader
            GLuint vert = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vert, 1, &vert_src, nullptr);
            glCompileShader(vert);

            // Compile fragment shader
            GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(frag, 1, &frag_src, nullptr);
            glCompileShader(frag);

            // Link shader program
            shader_program = glCreateProgram();
            glAttachShader(shader_program, vert);
            glAttachShader(shader_program, frag);
            glLinkProgram(shader_program);
            // Clean up shaders
            glDeleteShader(vert);
            glDeleteShader(frag);
        }

        // Enable blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Use shader
        glUseProgram(shader_program);

        // Set uniform color
        GLint color_loc = glGetUniformLocation(shader_program, "u_color");
        glUniform3f(color_loc,
            text.color.x / 255.0f,
            text.color.y / 255.0f,
            text.color.z / 255.0f
        );

        // Set texture uniform
        GLint tex_loc = glGetUniformLocation(shader_program, "u_texture");
        glUniform1i(tex_loc, 0); // Texture unit 0

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, text.font->glid);
        glBindVertexArray(textVAO);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);

        float screen_w = (float) window->get_size().x;
        float screen_h = (float) window->get_size().y;

        stbtt_fontinfo info;
        stbtt_InitFont(&info, text.font->ttf_data.data(), 0);
        int ascent;
        stbtt_GetFontVMetrics(&info, &ascent, nullptr, nullptr);

        float scale = stbtt_ScaleForPixelHeight(&info, text.size);
        float line_height = (ascent - text.font->line_height) * scale;
        float baseline = ascent * scale;
        float x = position.x;
        float y = position.y + baseline;

        for (char c : text.text) {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(
                text.font->cdata,
                text.font->sttex_size.x,
                text.font->sttex_size.y,
                c - 32,
                &x, &y,
                &q, 1
            );

            float x0 = (q.x0 / screen_w) * 2.0f - 1.0f;
            float y0 = 1.0f - (q.y0 / screen_h) * 2.0f;
            float x1 = (q.x1 / screen_w) * 2.0f - 1.0f;
            float y1 = 1.0f - (q.y1 / screen_h) * 2.0f;

            float vertices[6][4] = {
                { x0, y0, q.s0, q.t0 },
                { x0, y1, q.s0, q.t1 },
                { x1, y1, q.s1, q.t1 },

                { x0, y0, q.s0, q.t0 },
                { x1, y1, q.s1, q.t1 },
                { x1, y0, q.s1, q.t0 }
            };
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }

    void renderer_2d::draw_shader(shader_t shader, rocket::fbounding_box rect) {
glUseProgram(shader.glprogram);

glm::mat4 projection = glm::ortho(0.f, (float) window->size.x, (float) window->size.y, 0.f, -1.f, 1.f);

glm::mat4 transform = projection
    * glm::translate(glm::mat4(1.f), glm::vec3(rect.pos.x + rect.size.x/2, rect.pos.y + rect.size.y/2, 0.f))
    * glm::scale(glm::mat4(1.f), glm::vec3(rect.size.x, rect.size.y, 1.f));

glUniformMatrix4fv(glGetUniformLocation(shader.glprogram, "u_transform"), 1, GL_FALSE, glm::value_ptr(transform));
glUniform2f(glGetUniformLocation(shader.glprogram, "u_size"), rect.size.x, rect.size.y);
glUniform1f(glGetUniformLocation(shader.glprogram, "u_radius"), 0.f);

glBindVertexArray(rectVAO);
glDrawArrays(GL_TRIANGLES, 0, 6);
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

    void renderer_2d::draw_fps(vec2f_t pos) {
        std::string fps_text = "FPS: " + std::to_string(this->fps);
        static rocket::text_t fps = rocket::text_t(fps_text, 24, rocket::rgb_color::green());
        fps.text = fps_text;
        this->draw_text(fps, pos);
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

    int renderer_2d::get_current_fps() {
        return static_cast<int>(std::round(1.0 / get_delta_time()));
    }

    void renderer_2d::begin_scissor_mode(rocket::fbounding_box rect) {
        glEnable(GL_SCISSOR_TEST);

        glScissor(
            rect.pos.x,
            window->get_size().y - rect.pos.y - rect.size.y,
            rect.size.x,
            rect.size.y
        );
    }

    void renderer_2d::begin_scissor_mode(rocket::vec2f_t pos, rocket::vec2f_t size) {
        this->begin_scissor_mode({ pos, size });
    }

    void renderer_2d::begin_scissor_mode(float x, float y, float sx, float sy) {
        this->begin_scissor_mode({ { x, y }, { sx, sy } });
    }

    void renderer_2d::end_scissor_mode() {
        glDisable(GL_SCISSOR_TEST);
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
