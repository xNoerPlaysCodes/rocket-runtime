#include <GL/glew.h>
#include "rocket/io.hpp"
#include "rocket/rgl.hpp"
#include "rocket/runtime.hpp"
#include "rocket/types.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <unordered_map>
#include <utility>
#include "../include/rgl.hpp"
#include "../../include/rocket/renderer.hpp"
#include "util.hpp"
#include <GLFW/glfw3.h>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/vector_float3.hpp>
#include <string>
#include <thread>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace rocket {
    std::vector<shader_t> shader_cache;

    rgl::vao_t textVAO;
    rgl::vbo_t textVBO;

    rgl::fbo_t fxaa_fbo;
    rgl::shader_program_t fxaa_shader;

    renderer_2d::renderer_2d(window_t *window, int fps, renderer_flags_t flags) {
        this->window = window;
        this->fps = fps;

        glfwMakeContextCurrent(window->glfw_window);
        this->vsync = window->flags.vsync;

        if (!util::glinitialized()) {
            util::glinit(true);

            // Must make sure OpenGL context is current before glewInit
            if (!window || !window->glfw_window) {
                rocket::log_error("Invalid window ptr or window->glfw_window not initialized", -1, "renderer_2d::constructor", "fatal-to-function");
                return;
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
        this->flags = flags;
        if (flags.fxaa_simplified) {
            fxaa_fbo = rgl::create_fbo();
            fxaa_shader = rgl::get_fxaa_simplified_shader();
        }
        // replacement for gl_setup_ortho
        glViewport(0, 0, window->size.x, window->size.y);
    }

    void renderer_2d::draw_circle(rocket::vec2f_t pos, float radius, rocket::rgba_color color) {
        rocket::vec2f_t center_pos = {
            .x = pos.x - radius,
            .y = pos.y - radius
        };
        this->draw_rectangle({ center_pos, { radius * 2, radius * 2 } }, color, 0, 1);
    }
 

    void renderer_2d::draw_polygon(rocket::vec2f_t pos, float radius, rocket::rgba_color color, int segments, float rotation) {
        const char *vsrc = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;

            uniform vec4 uColor;
            out vec4 vColor;

            void main() {
                gl_Position = vec4(aPos, 0.0, 1.0);
                vColor = uColor;
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

        rocket::vec2f_t viewport = rgl::get_viewport_size();
        auto to_ndc = [&](float x, float y) {
            return rocket::vec2f_t{
                (x / viewport.x) * 2.0f - 1.0f,            // X: same
                1.0f - (y / viewport.y) * 2.0f             // Y: flipped
            };
        };

        std::pair<rgl::vao_t, rgl::vbo_t> vo = {0, 0};
        int vertex_count = 0;

        if (vo.first == 0) {
            std::vector<float> verts;
            verts.reserve((segments + 2) * 2);

            // center
            auto center = to_ndc(pos.x, pos.y);
            verts.push_back(center.x);
            verts.push_back(center.y);

            for (int i = 0; i <= segments; i++) {
                float angle = ((float)i / (float)segments) * 2.0f * M_PI;
                angle += rotation * (M_PI / 180.0f); // apply rotation in radians

                float px = pos.x + cosf(angle) * radius;
                float py = pos.y + sinf(angle) * radius;
                auto ndc = to_ndc(px, py);
                verts.push_back(ndc.x);
                verts.push_back(ndc.y);
            }

            vertex_count = (int)(verts.size() / 2);
            
            vo = rgl::compile_vo(verts);
        }
        static rgl::shader_program_t pg = rgl::cache_compile_shader(vsrc, fsrc);

        auto color_nm = color.normalize();
        rgl::shader_location_t color_loc = rgl::get_shader_location(pg, "uColor");

        glUseProgram(pg);
        glUniform4f(color_loc, color_nm.x, color_nm.y, color_nm.z, color_nm.w);

        // Make sure your draw call uses TRIANGLE_FAN
        glBindVertexArray(vo.first);
        glUseProgram(pg);
        rgl::gl_draw_arrays(GL_TRIANGLE_FAN, 0, vertex_count);
    }

    void renderer_2d::draw_pixel(rocket::vec2f_t pos, rocket::rgba_color color) {
        this->draw_rectangle({ pos, { 1, 1 } }, color, 0, 0, 0);
    }

    void renderer_2d::draw_line(rocket::vec2f_t start, rocket::vec2f_t end, rocket::rgba_color color, float thickness) {
        // Quick Note:
        //  I tried, every. single. algorithm.
        //  I MADE my own algorithm.
        //  I used EVERY algorithm that exists.
        //  Yet it STILL doesn't work.
        //  At this point, I'd like to quit here, and
        //  one of you smarties can get this working.
        //
        //  Sincerely,
        //  noerlol
        rocket::log_error("Implementation not finished", -1, "renderer_2d::draw_line", "fatal-to-function");
    }
    void renderer_2d::draw_rectangle(rocket::vec2f_t pos, rocket::vec2f_t size, rocket::rgba_color color, float rotation, float roundedness, bool lines) {
        rocket::fbounding_box box = {pos, size};
        this->draw_rectangle(box, color, rotation, roundedness, lines);
    }

    void renderer_2d::begin_frame() {
        start_time = std::chrono::high_resolution_clock::now();
        frame_start_time = glfwGetTime();
        delta_time = frame_start_time - last_time;
        last_time = frame_start_time;
    }

    rocket::rgba_color this_frame_clear_color = rgba_color::blank();

    void renderer_2d::begin_render_mode(render_mode_t mode) {
        if (mode == render_mode_t::fxaa) {
            if (!rgl::is_active_any_fbo()) {
                rgl::use_fbo(fxaa_fbo);
                auto color_nm = this_frame_clear_color.normalize();
                glClearColor(color_nm.x, color_nm.y, color_nm.z, color_nm.w);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }
            active_render_modes.push_back(mode);
        } else {
            rocket::log_error("Not Implemented Yet!", -1, "renderer_2d::begin_render_mode", "info");
        }
    }

    void renderer_2d::begin_batch() {
        this->batched = true;
        rocket::log_error("Batching is not implemented yet", -1, "renderer_2d", "info");
    }

    void renderer_2d::clear(rocket::rgba_color color) {
        this_frame_clear_color = color;

        vec4f_t clr = color.normalize();
        glClearColor(clr.x, clr.y, clr.z, clr.w);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
        static rgl::shader_program_t shader_program = rGL_SHADER_INVALID;
        if (shader_program == rGL_SHADER_INVALID) {
            // const char* vert_src = R"(
            //     #version 330 core
            //     layout(location = 0) in vec2 aPos;
            //     layout(location = 1) in vec2 aTex;
            //
            //     out vec2 TexCoord;
            //
            //     void main() {
            //         gl_Position = vec4(aPos.xy, 0.0, 1.0);
            //         TexCoord = aTex;
            //     }
            // )";
            //
            // const char* frag_src = R"(
            //     #version 330 core
            //     in vec2 TexCoord;
            //     out vec4 FragColor;
            //
            //     uniform sampler2D u_texture;
            //     uniform vec3 u_color;
            //
            //     void main() {
            //         float alpha = texture(u_texture, TexCoord).r;
            //
            //         // Gamma correct to linear
            //         alpha = pow(alpha, 2.2);
            //
            //         // Sharpen edges (smaller = sharper, 1.0 = no change)
            //         alpha = pow(alpha, 0.5);
            //
            //         // Convert back to sRGB
            //         alpha = pow(alpha, 1.0 / 2.2);
            //
            //         // Premultiply color
            //         vec3 rgb = u_color * alpha;
            //         FragColor = vec4(rgb, alpha);
            //     }
            // )";

            shader_program = rgl::get_shader(rgl::shader_use_t::text);
        }

        // Use shader
        glUseProgram(shader_program);

        // Set uniform color
        rgl::shader_location_t color_loc = glGetUniformLocation(shader_program, "u_color");
        glUniform3f(color_loc,
            text.color.x / 255.0f,
            text.color.y / 255.0f,
            text.color.z / 255.0f
        );

        // Set texture uniform
        rgl::shader_location_t tex_loc = glGetUniformLocation(shader_program, "u_texture");
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

    void draw_fullscreen_quad() {
        static rgl::vao_t vao = 0;
        if (vao == rGL_VAO_INVALID) {
            unsigned int vbo;
            float verts[] = {
                // x,    y,   u,  v
                -1.f, -1.f, 0.f, 0.f, // bottom-left
                 1.f, -1.f, 1.f, 0.f, // bottom-right
                -1.f,  1.f, 0.f, 1.f, // top-left
                 1.f,  1.f, 1.f, 1.f  // top-right
            };

            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);

            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

            // position
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

            // texcoord
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        }
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    void renderer_2d::end_render_mode(render_mode_t mode) {
        if (mode == render_mode_t::fxaa) {
            if (rgl::is_active_any_fbo()) {
                rgl::reset_to_default_fbo();
            }
        } else {
            rocket::log_error("Not Implemented Yet!", -1, "renderer_2d::end_render_mode", "info");
        }
    }

    void renderer_2d::end_frame() {
        if (flags.fxaa_simplified && fxaa_shader != rGL_SHADER_INVALID && std::find(active_render_modes.begin(), active_render_modes.end(), render_mode_t::fxaa) != active_render_modes.end()) {
            vec4f_t nm = this_frame_clear_color.normalize();
            glClearColor(nm.x, nm.y, nm.z, nm.w);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(fxaa_shader);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, fxaa_fbo.color_tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glUniform1i(rgl::get_shader_location(fxaa_shader, "uScene"), 0);
            glUniform2f(rgl::get_shader_location(fxaa_shader, "uResolution"), rgl::get_viewport_size().x, rgl::get_viewport_size().y);

            draw_fullscreen_quad();
        }
        glfwSwapBuffers(this->window->glfw_window);
        int drawcalls = rgl::reset_drawcalls();
        if (drawcalls >= rGL_MAX_RECOMMENDED_DRAWCALLS) {
            rocket::log_error("Too many drawcalls! (" + std::to_string(drawcalls) + ") Frames may suffer", -1, "renderer_2d::end_frame", "warning");
        }
        int tricount = rgl::reset_tricount();
        if (tricount >= rGL_MAX_RECOMMENDED_TRICOUNT) {
            rocket::log_error("Too many triangles! (" + std::to_string(tricount) + ") Frames may suffer", -1, "renderer_2d::end_frame", "warning");
        }

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

        if (fxaa_fbo != rGL_FBO_INVALID) {
            rgl::use_fbo(fxaa_fbo);
            rgl::update_viewport(final_viewport_position, final_viewport_size);
        }
        rgl::fbo_t fbo = rgl::get_active_fbo();
        rgl::reset_to_default_fbo();

        rgl::update_viewport(final_viewport_position, final_viewport_size);

        if (fbo != rGL_FBO_INVALID) {
            rgl::use_fbo(fbo);
            rgl::update_viewport(final_viewport_position, final_viewport_size);
        }

        frame_counter++;

        auto err = glGetError();
        if (err != GL_NO_ERROR) {
            rocket::log_error(reinterpret_cast<const char *>(glewGetErrorString(err)), err, "OpenGL", "fatal");
            this->window->close();
            std::exit(1);
        }

        this->active_render_modes.clear();

        if (this->fps == -1) {
            return;
        }

        if (this->vsync) {
            this->delta_time = glfwGetTime() - frame_start_time;
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

    uint64_t renderer_2d::get_framecount() {
        return this->frame_counter;
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
