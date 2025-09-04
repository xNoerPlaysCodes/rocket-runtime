#include <GL/glew.h>
#include "rocket/types.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <unordered_map>
#include <utility>
#define STB_TRUETYPE_IMPLEMENTATION
#include <iostream>
#include "../include/rgl.hpp"
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
#include <glm/gtc/type_ptr.hpp>           // for glm::value_ptr

namespace rocket {
    std::vector<shader_t> shader_cache;

    GLuint rectVAO, rectVBO;
    GLuint txVAO, txVBO;
    GLuint textVAO, textVBO;

    renderer_2d::renderer_2d(window_t *window, int fps) {
        this->window = window;
        this->fps = fps;
        this->vsync = false;

        glfwMakeContextCurrent(window->glfw_window);
        this->vsync = window->flags.vsync;

        if (!util::glinitialized()) {
            util::glinit(true);

            // Must make sure OpenGL context is current before glewInit
            if (!window || !window->glfw_window) {
                std::cerr << "[ERROR] Invalid window pointer or uninitialized!\n";
                std::exit(1);
            }
            glfwMakeContextCurrent(window->glfw_window);
            std::vector<std::string> log_messages = rgl::init_gl({ static_cast<float>(window->size.x), static_cast<float>(window->size.y) });

            std::pair<rgl::vao_t, rgl::vbo_t> text_vo = rgl::get_text_vos();
            textVAO = text_vo.first;
            textVBO = text_vo.second;

            for (auto &l : log_messages) {
                rocket::log(l, "renderer_2d", "constructor", "info");
            }
        }
        util::gl_setup_ortho(window->size);
    }

    void renderer_2d::draw_circle(rocket::vec2f_t pos, float radius, rocket::rgba_color color) {
        draw_rectangle({ pos, { radius * 2, radius * 2 } }, color, 0, 1);
    }

    void renderer_2d::draw_line(rocket::vec2f_t start, rocket::vec2f_t end, rocket::rgba_color color, float thickness) {
        // Direction vector
        rocket::vec2f_t dir = { end.x - start.x, end.y - start.y };

        // Line length
        float length = sqrt(dir.x * dir.x + dir.y * dir.y);

        // Rotation angle in radians
        float angle = atan2(dir.y, dir.x);

        // Rectangle representing the line
        rocket::fbounding_box box = {
            .pos = start,
            .size = { length, thickness }  // width = line length, height = line thickness
        };

        // Draw rotated rectangle (0 rounding for a normal line)
        draw_rectangle(box, color, glm::degrees(angle), 0.0f);
    }

    void renderer_2d::begin_frame() {
        start_time = std::chrono::high_resolution_clock::now();
        frame_start_time = glfwGetTime();
        delta_time = frame_start_time - last_time;
        last_time = frame_start_time;
    }

    void renderer_2d::begin_batch() {
        this->batched = true;
        rocket::log_error("Batching is not implemented yet", -1, "renderer_2d", "info");
    }

    void renderer_2d::clear(rocket::rgba_color color) {
        glGetError();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        vec4<float> clr = util::glify_a(color);
        glClearColor(clr.x, clr.y, clr.z, clr.w);
    }

    void renderer_2d::draw_texture(std::shared_ptr<rocket::texture_t> texture, rocket::fbounding_box rect, float rotation, float roundedness) {
        rgl::shader_program_t pg = rgl::get_paramaterized_textured_quad(rect.pos, rect.size, rotation, roundedness);
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
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture->glid);
        glUniform1i(glGetUniformLocation(pg, "u_texture"), 0);
        rgl::draw_shader(pg, rgl::shader_use_t::textured_rect);
    }

    void renderer_2d::draw_rectangle(rocket::fbounding_box rect, rocket::rgba_color color, float rotation, float roundedness, bool lines) {
        rgl::shader_program_t pg = rgl::get_paramaterized_quad(rect.pos, rect.size, color, rotation, roundedness);
        if (lines) {
            rocket::vec2f_t pos = rect.pos;
            rocket::vec2f_t size = rect.size;
            float thickness = 1.f; // hardcode TODO
            // top
            draw_rectangle({ pos, { size.x, thickness } }, color);
            // bottom
            draw_rectangle({ { pos.x, pos.y + size.y - thickness }, { size.x, thickness } }, color);
            // left
            draw_rectangle({ pos, { thickness, size.y } }, color);
            // right
            draw_rectangle({ { pos.x + size.x - thickness, pos.y }, { thickness, size.y } }, color);

            return;
        }
        if (this->batched) {
            instanced_quad_t i;
            i.pos = rect.pos;
            i.size = rect.size;
            i.color = color;
            i.gltxid = rGL_TXID_INVALID;
            this->batch.push_back(i);
            return;
        }
        
        rgl::draw_shader(pg, rgl::shader_use_t::rect);
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

        std::vector<float> verts;
        verts.reserve(text.text.size() * 6 * 4); // 6 vertices * 4 floats per vertex

        for (char c : text.text) {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(text.font->cdata, 512, 512, c - 32, &x, &y, &q, 1);

            float x0 = (q.x0 / screen_w) * 2.0f - 1.0f;
            float y0 = 1.0f - (q.y0 / screen_h) * 2.0f;
            float x1 = (q.x1 / screen_w) * 2.0f - 1.0f;
            float y1 = 1.0f - (q.y1 / screen_h) * 2.0f;

            float quad[6][4] = {
                { x0, y0, q.s0, q.t0 },
                { x0, y1, q.s0, q.t1 },
                { x1, y1, q.s1, q.t1 },

                { x0, y0, q.s0, q.t0 },
                { x1, y1, q.s1, q.t1 },
                { x1, y0, q.s1, q.t0 }
            };

            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad), quad);
            rgl::draw_shader(shader_program, rgl::shader_use_t::text);
        }
    }

    void renderer_2d::draw_shader(shader_t shader) {
        rgl::draw_shader(shader.glprogram, shader.vao, shader.vbo);
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

    void renderer_2d::set_viewport_size(vec2f_t size) {
        this->override_viewport_size = size;
    }

    void renderer_2d::set_viewport_offset(vec2f_t offset) {
        this->override_viewport_offset = offset;
    }

    void renderer_2d::draw_fps(vec2f_t pos) {
        std::string fps_text = "FPS: " + std::to_string(get_current_fps());

        static rocket::text_t fps = rocket::text_t(fps_text, 24, rocket::rgb_color::green());
        fps.text = fps_text;
        this->draw_text(fps, pos);
    }
}

namespace std {
    template<> struct hash<std::pair<float, float>> {
        size_t operator()(const std::pair<float, float> &p) const {
            return std::hash<float>()(p.first) ^ std::hash<float>()(p.second);
        }
    };
}

namespace rocket {
    std::unordered_map<std::pair<float, float>, std::vector<rgba_color>> fb_pixels;
    std::vector<rgba_color> renderer_2d::get_framebuffer() {
        std::pair<float, float> key = { rgl::get_viewport_size().x, rgl::get_viewport_size().y };
        auto it = fb_pixels.find(key);
        if (it != fb_pixels.end()) {
            return it->second;
        }

        fb_pixels[key] = std::vector<rgba_color>(rgl::get_viewport_size().x * rgl::get_viewport_size().y);
        return fb_pixels[key];
    }

    void renderer_2d::push_framebuffer(std::vector<rgba_color> &framebuffer) {
        static GLuint framebuffer_tx = rGL_TXID_INVALID;
        if (framebuffer_tx == rGL_TXID_INVALID) {
            glGenTextures(1, &framebuffer_tx);
            glBindTexture(GL_TEXTURE_2D, framebuffer_tx);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgl::get_viewport_size().x, rgl::get_viewport_size().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }

        std::vector<uint8_t> flat(framebuffer.size() * 4);
        std::memcpy(flat.data(), framebuffer.data(), framebuffer.size() * sizeof(rgba_color));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, framebuffer_tx);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, rgl::get_viewport_size().x, rgl::get_viewport_size().y, GL_RGBA, GL_UNSIGNED_BYTE, flat.data());

        static rgl::shader_program_t shader = rgl::get_paramaterized_textured_quad({0,0}, rgl::get_viewport_size(), 0.f, 0.f);
        glUniform1i(glGetUniformLocation(shader, "u_texture"), 0);
        rgl::draw_shader(shader, rgl::shader_use_t::textured_rect);

        framebuffer.clear();
    }

    vec2f_t renderer_2d::get_viewport_size() {
        return rgl::get_viewport_size();
    }

    void renderer_2d::end_frame() {
        glfwSwapBuffers(this->window->glfw_window);
        int _ = rgl::reset_drawcalls();

        rocket::vec2f_t final_viewport_position = { 0, 0 };
        rocket::vec2f_t final_viewport_size     = { -1, -1 };

        // If override size is set, use that, otherwise fallback to window size
        if (this->override_viewport_size != rocket::vec2f_t{ -1, -1 }) {
            final_viewport_size = this->override_viewport_size;
        } else {
            final_viewport_size = {
                static_cast<float>(this->window->size.x),
                static_cast<float>(this->window->size.y)
            };
        }

        // If override offset is set, use that
        if (this->override_viewport_offset != rocket::vec2f_t{ -1, -1 }) {
            final_viewport_position = this->override_viewport_offset;
        }

        rgl::update_viewport(final_viewport_position, final_viewport_size);

        frame_counter++;

        auto err = glGetError();
        if (err != GL_NO_ERROR) {
            rocket::log_error(reinterpret_cast<const char *>(glewGetErrorString(err)), err, "OpenGL", "fatal");
            this->window->close();
            std::exit(1);
        }

        if (this->fps == -1) {
            return;
        }

        if (this->vsync) {
            return; // We're done here
        }

        double frame_end_time = glfwGetTime();
        double frame_duration = frame_end_time - frame_start_time;

        double frametime_limit = 1.0 / fps;
        if (frame_duration < frametime_limit) {
            double sleep_time = frametime_limit - frame_duration;

            // sleep for most of it
            if (sleep_time > 0.002) // leave ~2ms for busy-wait
                std::this_thread::sleep_for(std::chrono::duration<double>(sleep_time - 0.002));

            // busy wait for the rest
            while ((glfwGetTime() - frame_start_time) < frametime_limit) {}
        }

        this->delta_time = glfwGetTime() - frame_start_time;
        this->frame_start_time = glfwGetTime();
    }

    struct batched_quad_t {
        vec2f_t pos = {0,0};
        vec2f_t size = {0,0};
        vec4f_t color = { 0,0,0,0 };
        GLuint gltxid;
    };

    void init_batch_renderer(rgl::vao_t quadVAO, rgl::vbo_t quadVBO, rgl::vbo_t instanceVBO) {
        // Base quad (two triangles, 0..1 range)
        float quadVertices[] = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,

            0.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glGenBuffers(1, &instanceVBO);

        glBindVertexArray(quadVAO);

        // base quad VBO
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

        // instance VBO (empty init)
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, 2048 /* max batch size */ * sizeof(batched_quad_t), nullptr, GL_DYNAMIC_DRAW);

        // iPos
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(batched_quad_t), (void*)offsetof(batched_quad_t, pos));
        glVertexAttribDivisor(1, 1);

        // iSize
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(batched_quad_t), (void*)offsetof(batched_quad_t, size));
        glVertexAttribDivisor(2, 1);

        // iColor
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(batched_quad_t), (void*)offsetof(batched_quad_t, color));
        glVertexAttribDivisor(3, 1);

        glBindVertexArray(0);
    }

    void renderer_2d::end_batch(size_t max_batch_size) {
        if (batch.size() > max_batch_size || batch.size() == 0) {
            this->batch.clear();
            return;
        }

        enum class batch_type {
            quad = 0,
            txquad = 1
        };

        std::vector<batched_quad_t> bquads;

        batch_type type = batch_type::txquad;
        if (batch.at(0).gltxid == rGL_TXID_INVALID) {
            type = batch_type::quad;
        }

        static rgl::vao_t quadVAO;
        static rgl::vbo_t quadVBO;
        static rgl::vbo_t instanceVBO;

        if (quadVAO == 0) {
            init_batch_renderer(quadVAO, quadVBO, instanceVBO);
        }

        for (auto &i : batch) {
            batched_quad_t bq;
            vec2f_t nmpos = {};
            vec2f_t nmsize = {};

            nmpos.x = i.pos.x / window->size.x;
            nmpos.y = i.pos.y / window->size.y;
            nmsize.x = i.size.x / window->size.x;
            nmsize.y = i.size.y / window->size.y;

            bq.pos = nmpos;
            bq.size = nmsize;

            bq.color = i.color.normalize();
            bq.gltxid = i.gltxid;
            bquads.push_back(bq);
        }

        static rgl::shader_program_t pg = 0;
        if (pg == 0) {
            const char *vsrc = R"(
                #version 330 core
                in vec2 aPos;        // quad vertex
                in vec2 iPos;        // instance position
                in vec2 iSize;       // instance size
                in vec4 iColor;      // instance color

                out vec4 vColor;

                void main() {
                    vec2 pos = aPos * iSize + iPos;
                    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
                    vColor = iColor;
                }
            )";

            const char *fsrc = R"(
                #version 330 core
                in vec4 vColor;
                out vec4 FragColor;
                void main() {
                    FragColor = vColor;
                }
            )";
            pg = rgl::cache_compile_shader(vsrc, fsrc);
        }


        // glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        // glBufferSubData(GL_ARRAY_BUFFER, 0, batch.size() * sizeof(batched_quad_t), bquads.data());

        glUseProgram(pg);
        glBindVertexArray(quadVAO);

        // data
        rgl::shader_location_t aPos = rgl::get_shader_location(pg, "aPos");
        rgl::shader_location_t iPos = rgl::get_shader_location(pg, "iPos");
        rgl::shader_location_t iSize = rgl::get_shader_location(pg, "iSize");
        rgl::shader_location_t iColor = rgl::get_shader_location(pg, "iColor");

        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, bquads.size() * sizeof(batched_quad_t), bquads.data(), GL_DYNAMIC_DRAW);

        // iPos
        glEnableVertexAttribArray(iPos);
        glVertexAttribPointer(iPos, 2, GL_FLOAT, GL_FALSE, sizeof(batched_quad_t), (void*)offsetof(batched_quad_t, pos));
        glVertexAttribDivisor(iPos, 1);

        // iSize
        glEnableVertexAttribArray(iSize);
        glVertexAttribPointer(iSize, 2, GL_FLOAT, GL_FALSE, sizeof(batched_quad_t), (void*)offsetof(batched_quad_t, size));
        glVertexAttribDivisor(iSize, 1);

        // iColor
        glEnableVertexAttribArray(iColor);
        glVertexAttribPointer(iColor, 4, GL_FLOAT, GL_FALSE, sizeof(batched_quad_t), (void*)offsetof(batched_quad_t, color));
        glVertexAttribDivisor(iColor, 1);

        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, bquads.size());

        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, batch.size());
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

    int renderer_2d::get_drawcalls() {
        return rgl::read_drawcalls();
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
            rocket::log_error("window already closed", -1, "RocketRuntime", "warn");
            return;
        }
        window->close();
    }

    renderer_2d::~renderer_2d() {
        close();
    }
}
