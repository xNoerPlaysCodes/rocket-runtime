#include "rocket/audio.hpp"
#include <GL/glew.h>
#include <shader_provider.hpp>
#define RGL_EXPOSE_NATIVE_LIB
#include "rocket/rgl.hpp"
#include "rocket/asset.hpp"
#include "rocket/io.hpp"
#include "rocket/plugin/plugin.hpp"
#include "rocket/runtime.hpp"
#include "rocket/types.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <iostream>
#include <unordered_map>
#include <utility>
#include "rgl.hpp"
#include "rocket/renderer.hpp"
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
#include "tweeny.hpp"

#include "binary_stuff/splash_screen.h"
#include "binary_stuff/splash_sfx.h"

#include "plugin.hpp"

namespace rocket {
    std::vector<shader_t> shader_cache;

    rgl::fbo_t fxaa_fbo = rGL_FBO_INVALID;
    rgl::shader_program_t fxaa_shader;
    
    util::global_state_cliargs_t ovr_clistate;

    renderer_2d::renderer_2d(window_t *window, int fps, renderer_flags_t flags) {
        this->window = window;
        this->fps = fps;

        auto cli_args = util::get_clistate();
        if (cli_args.dx11 || cli_args.dx12) {
            rocket::log("D3D11/12 backend not supported", "renderer_2d", "constructor", "fatal");
            rocket::exit(1);
        }

        if (cli_args.framerate != -1) {
            this->fps = cli_args.framerate;
        }

        if (flags.share_renderer_as_global) {
            util::set_global_renderer_2d(this);
        }

        glfwMakeContextCurrent(window->glfw_window);
        this->vsync = window->flags.vsync;

        if (!util::glinitialized()) {
            util::glinit(true);

            if (!window || !window->glfw_window) {
                rocket::log("Invalid window ptr", "renderer_2d", "constructor", "error");
                return;
            }
            glfwMakeContextCurrent(window->glfw_window);
            std::vector<std::string> log_messages = rgl::init_gl({ static_cast<float>(window->size.x), static_cast<float>(window->size.y) });

            for (auto &l : log_messages) {
                if (l.starts_with('!')) 
                    rocket::log(l.substr(1), "rgl", "init_gl", "warn");
                else if (l.starts_with('?'))
                    rocket::log(l.substr(1), "rgl", "init_gl", "error");
                else
                    rocket::log(l, "rgl", "init_gl", "info");
            }

            rocket::logger_flush();
        }
        this->flags = flags;
        if (flags.fxaa_simplified) {
            fxaa_fbo = rgl::create_fbo();
            fxaa_shader = rgl::get_fxaa_simplified_shader();
        }

        glViewport(0, 0, window->size.x, window->size.y);

        ::rocket::ovr_clistate = util::get_clistate();
    }

    void renderer_2d::draw_circle(rocket::vec2f_t pos, float radius, rocket::rgba_color color, int thickness) {
        rocket::vec2f_t center_pos = {
            .x = pos.x - radius,
            .y = pos.y - radius
        };

        if (thickness > 0) {
            rgl::shader_program_t pg = rocket::get_shader(shader_id_t::circle_lines);
            rocket::vec2f_t viewport_size = rgl::get_viewport_size();
            glm::mat4 projection = glm::ortho(0.f, viewport_size.x, viewport_size.y, 0.f, -1.f, 1.f);

            float cx = center_pos.x + radius * 2 * 0.5f;
            float cy = center_pos.y + radius * 2 * 0.5f;

            glm::mat4 transform = projection
                * glm::translate(glm::mat4(1.0f), glm::vec3(cx, cy, 0.0f))
                * glm::rotate(glm::mat4(1.0f), glm::radians(0.f), glm::vec3(0.0f, 0.0f, 1.0f))
                * glm::translate(glm::mat4(1.0f), glm::vec3(-radius * 2 * 0.5f, -radius * 2 * 0.5f, 0.0f))
                * glm::scale(glm::mat4(1.0f), glm::vec3(radius * 2, radius * 2, 1.0f));

            auto nm = color.normalize();

            glUseProgram(pg);
            glUniformMatrix4fv(glGetUniformLocation(pg, "u_transform"), 1, GL_FALSE, glm::value_ptr(transform));
            glUniform4f(glGetUniformLocation(pg, "u_color"), nm.x, nm.y, nm.z, nm.w);
            glUniform2f(glGetUniformLocation(pg, "u_size"), radius * 2, radius * 2);
            glUniform1f(glGetUniformLocation(pg, "u_radius"), 1);
            glUniform1f(glGetUniformLocation(pg, "u_thickness"), thickness);
            auto vos = rgl::compile_vo();
            rgl::draw_shader(pg, vos.first, vos.second);
            return;
        }

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
                1.0f - (y / viewport.y) * 2.0f             // Y: flip
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

        glBindVertexArray(vo.first);
        glUseProgram(pg);
        rgl::gl_draw_arrays(GL_TRIANGLE_FAN, 0, vertex_count);
    }

    void renderer_2d::draw_pixel(rocket::vec2f_t pos, rocket::rgba_color color) {
        this->draw_rectangle({ pos, { 1, 1 } }, color, 0, 0, 0);
    }

    void renderer_2d::draw_rectangle(rocket::vec2f_t pos, rocket::vec2f_t size, rocket::rgba_color color, float rotation, float roundedness, bool lines) {
        rocket::fbounding_box box = {pos, size};
        this->draw_rectangle(box, color, rotation, roundedness, lines);
    }

    void renderer_2d::begin_frame() {
        if (flags.show_splash && (!this->splash_shown)) {
            this->show_splash();
        }
        if (this->frame_counter == 0) {
            if (!window->flags.hidden) {
                glfwShowWindow(window->glfw_window);
            }
        }
        this->frame_started = true;
        start_time = std::chrono::high_resolution_clock::now();
        frame_start_time = glfwGetTime();
        delta_time = frame_start_time - last_time;
        last_time = frame_start_time;
    }

    void renderer_2d::show_splash() {
        static auto cli_args = util::get_clistate();
        if (cli_args.nosplash) {
            this->splash_shown = true;
            return;
        }
        this->splash_shown = true;

        float duration = 16384;

        auto tween = tweeny::from(0).to(0).during(duration / 2).via(tweeny::easing::cubicOut);

        asset_manager_t am;
        std::vector<uint8_t> splash_screen = std::vector<uint8_t>(splash_screen_png, splash_screen_png + splash_screen_png_len);
        auto tx = am.get_texture(am.load_texture(splash_screen));

        std::vector<uint8_t> splash_sfx = std::vector<uint8_t>(splash_sfx_ogg, splash_sfx_ogg + splash_sfx_ogg_len);
        auto aud = am.get_audio(am.load_audio(splash_sfx));

        bool final = false;

        bool splash_finished = false;

        auto size = window->size;

        aud->play();

        while (window->is_running() && !splash_finished) {
            if (window->get_size() != size) {
                window->set_size(size);
            }
            this->begin_frame();
            this->clear({ 0, 0, 0, 255 });
            {
                float alpha = tween.step((float) this->get_delta_time());
                float center_x = get_viewport_size().x / 2 - window->get_size().y / 2;

                this->draw_texture(tx, { {center_x, 0}, { window->get_size().y * 1.f, window->get_size().y * 1.f } });

                rocket::text_t version_text = { "Version: " ROCKETGE__VERSION, 24, rgb_color::white(), rGE__FONT_DEFAULT_MONOSPACED };

                this->draw_rectangle({ 0, 0 }, this->get_viewport_size(), { 0, 0, 0, (uint8_t) alpha });

                this->draw_text(version_text, { 0, get_viewport_size().y - version_text.measure().y });


                if (tween.progress() >= 1.f) {
                    if (final) {
                        splash_finished = true;
                    }
                    final = true;
                    tween = tweeny::from(0).to(255).during(duration).via(tweeny::easing::cubicOut);
                }
            }
            this->end_frame();
            this->window->poll_events();
        }
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
            rocket::log("Not Implemented Yet!", "renderer_2d", "begin_render_mode", "info");
        }
    }

    void renderer_2d::begin_batch() {
        this->batched = true;
        rocket::log("Batching is not implemented yet", "renderer_2d", "begin_batch", "info");
    }

    void renderer_2d::clear(rocket::rgba_color color) {
        this_frame_clear_color = color;

        vec4f_t clr = color.normalize();
        glClearColor(clr.x, clr.y, clr.z, clr.w);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // you probably want plugins to start rendering AFTER clearing
        if (flags.share_renderer_as_global) {
            __rallframestart();
        }
    }

    void renderer_2d::make_ready_texture(std::shared_ptr<rocket::texture_t> texture) {
        if (texture != nullptr && texture->glid == 0) {
            glGenTextures(1, &texture->glid);
            glBindTexture(GL_TEXTURE_2D, texture->glid);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, texture->size.x, texture->size.y, 0,
                         texture->channels == 4 ? GL_RGBA : GL_RGB,
                         GL_UNSIGNED_BYTE, texture->data.data());

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
    }

    void renderer_2d::draw_texture(std::shared_ptr<rocket::texture_t> texture, rocket::fbounding_box rect, float rotation, float roundedness) {
        if (texture == nullptr) {
            rocket::log("texture is null", "renderer_2d", "draw_texture", "error");
            return;
        }
        rgl::shader_program_t pg = rgl::get_paramaterized_textured_quad(rect.pos, rect.size, rotation, roundedness);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture->glid);
        this->make_ready_texture(texture);
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
        static auto cli_args = util::get_clistate();
        if (cli_args.notext) {
            return;
        }
        rgl::shader_program_t shader_program = rGL_SHADER_INVALID;
        if (shader_program == rGL_SHADER_INVALID) {
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
        auto text_vo = rgl::get_text_vos();
        glBindVertexArray(text_vo.first);
        glBindBuffer(GL_ARRAY_BUFFER, text_vo.second);

        float screen_w = (float) window->get_size().x;
        float screen_h = (float) window->get_size().y;

        stbtt_fontinfo info;
        stbtt_InitFont(&info, text.font->ttf_data.data(), 0);
        int ascent;
        stbtt_GetFontVMetrics(&info, &ascent, nullptr, nullptr);

        float scale = stbtt_ScaleForPixelHeight(&info, text.size);
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

    void renderer_2d::set_graphics_settings(graphics_settings_t settings) {
        this->graphics_settings = settings;
    }

    void renderer_2d::set_viewport_size(vec2f_t size) {
        this->override_viewport_size = size;
    }

    void renderer_2d::set_viewport_offset(vec2f_t offset) {
        this->override_viewport_offset = offset;
    }

    void renderer_2d::set_camera(camera_2d *cam) {
        rocket::log("cameras are not implemented", "renderer_2d", "set_camera", "fixme");
        this->cam = cam;
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
    std::vector<rgba_color> renderer_2d::get_framebuffer() {
        static std::unordered_map<std::pair<float, float>, std::vector<rgba_color>> fb_pixels;
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
            rocket::log("Not Implemented Yet!", "renderer_2d", "end_render_mode", "info");
        }
    }

    void draw_debug_overlay(bool draw, renderer_2d *ren, double start) {
        if (!draw) { return; }

        const float margin = 8.f;
        const float padding = 8.f;
        vec2f_t size = { 384, 262 };
        vec2f_t position = { margin, margin };

        const float zx = margin + padding;
        const float zy = margin + padding;
        const float text_size = 24;

        auto double_to_str = [](double d, int decimal_places = 4) -> std::string {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(decimal_places) << d;
            return ss.str();
        };
        static std::shared_ptr<rocket::font_t> font = rGE__FONT_DEFAULT_MONOSPACED;
        rocket::text_t fps_avg_text = { "FPS: " + std::to_string(ren->get_current_fps()), text_size, rgb_color::white(), font };
        rocket::text_t frametime_text = { "FrameTime: [[FIXME]]", text_size, rgb_color::white(), font }; // FIXME Get proper (ema avg) frametime
        rocket::text_t deltatime_text = { "DeltaTime: " + std::to_string(ren->get_delta_time()), text_size, rgb_color::white(), font };
        rocket::text_t drawcalls_text = { "Drawcalls: " + std::to_string(rgl::read_drawcalls()), text_size, rgb_color::white(), font };
        rocket::text_t tricount_text = { "TriCount: " + std::to_string(rgl::read_tricount()), text_size, rgb_color::white(), font };

        if (rgl::read_drawcalls() > rGL_MAX_RECOMMENDED_DRAWCALLS) {
            drawcalls_text.text += " (danger)";
        }

        if (rgl::read_tricount() > rGL_MAX_RECOMMENDED_TRICOUNT) {
            tricount_text.text += " (danger)";
        }

        std::string fbo_active = rgl::is_active_any_fbo() ? "Yes" : "No";
        rocket::text_t framebuffer_active_text = { "FBO Active: " + fbo_active, text_size, rgb_color::white(), font };
        vec2d_t dmpos = io::mouse_pos();
        vec2i_t mpos = {
            static_cast<int>(dmpos.x),
            static_cast<int>(dmpos.y)
        };
        rocket::text_t mouse_pos_text = { "Cursor Pos: " + std::to_string(mpos.x) + ", " + std::to_string(mpos.y), text_size, rgb_color::white(), font };

        rocket::text_t rocket_version_text = { "Engine Version: " + std::string(ROCKETGE__VERSION), text_size, rgb_color::white(), font };
        static std::string glmajor, glminor;
        static int mj = -1, mn = -1;
        if (mj == -1 || mn == -1) {
            glGetIntegerv(GL_MAJOR_VERSION, &mj);
            glGetIntegerv(GL_MINOR_VERSION, &mn);

            glmajor = std::to_string(mj);
            glminor = std::to_string(mn);
        }
        int versions[][19] = {
            {4,6},
            {4,5},
            {4,4},
            {4,3},
            {4,2},
            {4,1},
            {4,0},

            {3,3},
            {3,2},
            {3,1},
            {3,0},

            {2,1},
            {2,0},

            {1,5},
            {1,4},
            {1,3},
            {1,2},
            {1,1},
            {1,0}
        };

        static auto cli_args = util::get_clistate();

        static bool may_use_gl_major_minor_version = false;
        static std::string gl_rpt_version_string;
        if (gl_rpt_version_string.empty()) {
            gl_rpt_version_string = reinterpret_cast<const char*>(glGetString(GL_VERSION));
            gl_rpt_version_string = gl_rpt_version_string.substr(0, 3);
        }
        for (auto &v : versions) {
            if (v[0] == mj && v[1] == mn) {
                may_use_gl_major_minor_version = true;
                break;
            }
        }
        static bool gl_version_set = false;
        static std::string gl_version;
        if (!gl_version_set) {
            gl_version_set = true;
            if (may_use_gl_major_minor_version) {
                gl_version = glmajor + "." + glminor + " (core)";
            } else {
                gl_version = gl_rpt_version_string + " (compat)";
            }

            if (cli_args.glversion != GL_VERSION_UNK) {
                gl_version = double_to_str(cli_args.glversion, 1);
                if (cli_args.glversion < 3.0) {
                    gl_version += " (compat)";
                } else {
                    gl_version += " (core)";
                }
            }
        }
        rocket::text_t opengl_version_text = { "OpenGL Version: " + gl_version, text_size, rgb_color::white(), font };

        auto last_text_position = zy + size.y - text_size - padding - margin;

        static float max_width = std::max({fps_avg_text.measure().x, frametime_text.measure().x, deltatime_text.measure().x, drawcalls_text.measure().x,
                    tricount_text.measure().x, framebuffer_active_text.measure().x, mouse_pos_text.measure().x, rocket_version_text.measure().x, opengl_version_text.measure().x});
        size.x = max_width + 24.f;
        ren->draw_rectangle(position, size, rgba_color::black(), 0., 0.);
        ren->draw_rectangle(position, size, rgba_color::white(), 0., 0., true);
        ren->begin_scissor_mode(position, size);
        ren->draw_text(fps_avg_text, { zx, zy + (0 * text_size) });
        ren->draw_text(frametime_text, { zx, zy + (1 * text_size) });
        ren->draw_text(deltatime_text, { zx, zy + (2 * text_size) });
        ren->draw_text(drawcalls_text, { zx, zy + (3 * text_size) });
        ren->draw_text(tricount_text, { zx, zy + (4 * text_size) });
        ren->draw_text(framebuffer_active_text, { zx, zy + (5 * text_size) });
        ren->draw_text(mouse_pos_text, { zx, zy + (6 * text_size) });

        ren->draw_text(rocket_version_text, { zx, last_text_position });
        ren->draw_text(opengl_version_text, { zx, last_text_position - text_size });

        ren->end_scissor_mode();
    }

    std::unordered_map<std::pair<float, float>, rgl::texture_id_t> scraped_vps;

    bool renderer_2d::has_frame_began() {
        return this->frame_started;
    }

    bool renderer_2d::has_frame_ended() {
        return !this->frame_started;
    }

    void renderer_2d::end_frame() {
        this->frame_started = false;
        if (flags.share_renderer_as_global) {
            __rallframeend();
        }
        bool fxaa_on = flags.fxaa_simplified && fxaa_shader != rGL_SHADER_INVALID && std::find(active_render_modes.begin(), active_render_modes.end(), render_mode_t::fxaa) != active_render_modes.end();
        if (fxaa_on) { // [FIXME] Text drawing doesn't work
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

        draw_debug_overlay(util::get_clistate().debugoverlay, this, frame_start_time);

        glfwSwapBuffers(this->window->glfw_window);
        int drawcalls = rgl::reset_drawcalls();
        if (drawcalls > rGL_MAX_RECOMMENDED_DRAWCALLS) {
            rocket::log("Too many drawcalls! (" + std::to_string(drawcalls) + ") Frames may suffer", "renderer_2d", "end_frame", "warning");
        }
        int tricount = rgl::reset_tricount();
        if (tricount > rGL_MAX_RECOMMENDED_TRICOUNT) {
            rocket::log("Too many triangles! (" + std::to_string(tricount) + ") Frames may suffer", "renderer_2d", "end_frame", "warning");
        }

        rocket::vec2f_t final_viewport_position = {  0,  0 };
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

        static auto cli_args = util::get_clistate();

        if (fxaa_fbo != rGL_FBO_INVALID) {
            rgl::use_fbo(fxaa_fbo);
            if (!cli_args.viewport_size_set) {
                rgl::update_viewport(final_viewport_position, final_viewport_size);
            }
        }
        rgl::fbo_t fbo = rgl::get_active_fbo();
        rgl::reset_to_default_fbo();

        if (!cli_args.viewport_size_set) {
            rgl::update_viewport(final_viewport_position, final_viewport_size);
        }

        if (fbo != rGL_FBO_INVALID) {
            rgl::use_fbo(fbo);
            if (!cli_args.viewport_size_set) {
                rgl::update_viewport(final_viewport_position, final_viewport_size);
            }
        }

        if (scraped_vps.size() > 16) {
            scraped_vps.clear();
        }

        auto it = scraped_vps.find({ final_viewport_size.x, final_viewport_size.y });
        if (it == scraped_vps.end()) {
            std::pair<float, float> key = { final_viewport_size.x, final_viewport_size.y };
            rgl::texture_id_t tex;
            glGenTextures(1, &tex);
        }

        auto tx = scraped_vps[{ final_viewport_size.x, final_viewport_size.y }];
        glBindTexture(GL_TEXTURE_2D, tx);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, final_viewport_size.x, final_viewport_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, final_viewport_size.x, final_viewport_size.y);

        frame_counter++;

        this->active_render_modes.clear();

        if (this->fps == rocket::fps_uncapped) {
            this->delta_time = glfwGetTime() - frame_start_time;
            return;
        }

        if (this->vsync) {
            this->delta_time = glfwGetTime() - frame_start_time;
            return; // We're done here
        }

        double frame_end_time = glfwGetTime();
        double frame_duration = frame_end_time - frame_start_time;

        rgl::update_draw_metrics_data(frame_duration, 1 / this->delta_time);

        if (fps == 0) {
            rocket::log("Target FPS 0 is too low", "renderer_2d", "end_frame", "fatal");
            rocket::exit(1);
        }

        double frametime_limit = 1.0 / (fps + 0);
        if (frame_duration < frametime_limit) {
            double sleep_time = frametime_limit - frame_duration;

            if (sleep_time > 0.002)
                std::this_thread::sleep_for(std::chrono::duration<double>(sleep_time - 0.002));

            // busy wait for the rest
            while ((glfwGetTime() - frame_start_time) < frametime_limit) {}
        } /*else*/ {
            double diff = frame_duration - frametime_limit;

            auto double_to_str = [](double d, int decimal_places = 6) -> std::string {
                std::stringstream ss;
                ss << std::fixed << std::setprecision(decimal_places) << d;
                return ss.str();
            };

            const double threshold = 0.05;
            if (diff > threshold) {
                rocket::log("frame took " + double_to_str(diff * 1000., 2) + "ms more than expected", "renderer_2d", "end_frame", "debug");
            }
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

        [[maybe_unused]] batch_type type = batch_type::txquad;
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
        [[maybe_unused]] rgl::shader_location_t aPos = rgl::get_shader_location(pg, "aPos");
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
        return static_cast<int>(std::round(rgl::get_draw_metrics().avg_fps));
        // return static_cast<int>(std::round(1.0 / get_delta_time()));
    }

    int renderer_2d::get_drawcalls() {
        return rgl::read_drawcalls();
    }

    rgl::draw_metrics_t renderer_2d::get_draw_metrics() {
        return rgl::get_draw_metrics();
    }

    const graphics_settings_t &renderer_2d::get_graphics_settings() {
        return this->graphics_settings;
    }

    rgl::scoped_gl_texture_t renderer_2d::get_framebuffer_texture() {
        rgl::scoped_gl_texture_t t;
        glBindTexture(GL_TEXTURE_2D, t.id);

        int w = rgl::get_viewport_size().x;
        int h = rgl::get_viewport_size().y;

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, w, h);

        return t;
    }

    camera_2d* renderer_2d::get_camera() {
        rocket::log("cameras are not implemented", "renderer_2d", "get_camera", "fixme");
        return this->cam;
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
        window = nullptr; // unbind window

        if (util::get_global_renderer_2d() == this) {
            util::set_global_renderer_2d(nullptr);
        }

        rgl::cleanup_all();
    }

    renderer_2d::~renderer_2d() {
        close();
    }
}
