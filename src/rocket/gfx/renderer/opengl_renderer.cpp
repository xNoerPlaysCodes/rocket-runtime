#include "rocket/macros.hpp"
#if defined(ROCKETGE__Platform_Android)
    #include <GLES3/gl32.h>
    #include <EGL/egl.h>
#else
    #include <lib/glad/glad.h>
#endif
#include <shader_provider.hpp>
#include "rocket/rgl.hpp"
#include "rgl.hpp"
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
#include <glm/detail/qualifier.hpp>
#include <glm/ext/vector_float3.hpp>
#include <string>
#include <thread>
#include <vector>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "lib/tweeny/tweeny.h"

#include "binary_stuff/splash_screen.h"
#include "lib/stb/stb_image.h"
#include "lib/stb/stb_truetype.h"
#include "internal_types.hpp"

#include "plugin.hpp"
#include <intl_macros.hpp>

namespace rocket {
    unsigned int render_cache_t::get_texture() {
        return rGL_TXID_INVALID; // FIXME real fbo color tex
        // return this->fbo.color_tex;
    }
}

namespace rocket {
    util::global_state_cliargs_t ovr_clistate;

    opengl_renderer_2d::opengl_renderer_2d(window_backend_i *window, int fps, renderer_flags_t flags)
    {
        r_assert(window != nullptr);
        r_assert(fps != 0);

        this->impl = new renderer_2d_impl_t;
        this->impl->obj = this;
        this->bk_impl = new opengl_renderer_2d_impl_t;
        window->wbi_impl->bound_renderer2d = this;
        this->window = window;
        this->fps = fps;

        auto cli_args = util::get_clistate();

        if (cli_args.framerate != -1) {
            this->fps = cli_args.framerate;
        } 

        if (this->fps == -1) {
            this->fps = 2147483647;
        }

        if (flags.share_renderer_as_global) {
            util::set_global_renderer_2d(this);
        }

        this->vsync = window->flags.vsync;

        if (!util::glinitialized()) {
            util::timer_t gl_init_timer;
            util::glinit(true);

            if (window == nullptr) {
                rocket::log("Invalid window ptr", "opengl_renderer_2d", "constructor", "error");
                return;
            }
            std::vector<std::string> log_messages = rgl::init_gl(
                { static_cast<float>(window->size.x), static_cast<float>(window->size.y) },
                flags.glfnldr_backend,
                this->window
            );

            for (auto &l : log_messages) {
                if (l.starts_with('!')) 
                    rocket::log(l.substr(1), "rgl", "init_gl", "warn");
                else if (l.starts_with('?'))
                    rocket::log(l.substr(1), "rgl", "init_gl", "error");
                else
                    rocket::log(l, "rgl", "init_gl", "info");
            }

            rocket::logger_flush();
            gl_init_timer.stop();
            int gl_init_timer_ms;
            std::string unit;
            if (gl_init_timer.ms() > 1) {
                gl_init_timer_ms = static_cast<int>(std::round(gl_init_timer.ms()));
                unit = "ms";
            } else {
                gl_init_timer_ms = static_cast<int>(gl_init_timer.us());
                unit = "us";
            }
            rocket::log("OpenGL Initialized in " + std::to_string(gl_init_timer_ms) + unit, "opengl_renderer_2d", "constructor", "trace");
        }
        this->flags = flags;
        glViewport(0, 0, window->size.x, window->size.y);

        ::rocket::ovr_clistate = util::get_clistate();

        if (flags.show_splash && (!this->splash_shown)) {
            this->show_splash();
        }
    }
    
    opengl_renderer_2d::gfx_chk_result opengl_renderer_2d::check_graphics_settings(rocket::vec2f_t pos, rocket::vec2f_t sz) {
        if (!this->frame_started) return gfx_chk_result::not_drawable;
        if (this->graphics_settings.viewport_culling) {
            if (pos == rocket::vec2f_t{-1,-1} || sz == rocket::vec2f_t{-1,-1})
                return gfx_chk_result::drawable;

            rocket::fbounding_box obj = {pos, sz};
            rocket::fbounding_box vp = {{0,0}, this->get_viewport_size()};

            if (!obj.intersects(vp))
                return gfx_chk_result::not_drawable;

            return gfx_chk_result::drawable;
        }
        return gfx_chk_result::drawable;
    }

    api_object_t opengl_renderer_2d::upload_font_texture_to_gpu(const rocket::vec2i_t sz, const std::vector<uint8_t> &bitmap) {
        api_object_t handle = ++this->impl->current_object_handle;
        gl_object_t obj = {};
        obj.type = gl_object_type_t::texture;

        glGenTextures(1, &obj.value);
        glBindTexture(GL_TEXTURE_2D, obj.value);
#ifdef ROCKETGE__Platform_Android
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, sz.x, sz.y, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, bitmap.data());
#else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, sz.x, sz.y, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.data());
#endif

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        this->bk_impl->objects[handle] = obj;

        return handle;
    }

    void opengl_renderer_2d::clean_gpu_resource(api_object_t obj) {
        for (auto &[k, v] : bk_impl->objects) {
            if (k == obj) {
                if (v.type == gl_object_type_t::texture) {
                    glDeleteTextures(1, &v.value);
                }
                bk_impl->objects.erase(obj);
                break;
            }
        }
    }


    void opengl_renderer_2d::draw_circle(rocket::vec2f_t pos, float radius, rocket::rgba_color color, int thickness) {
        rocket::vec2f_t center_pos = {
            .x = pos.x - radius,
            .y = pos.y - radius
        };
        if (this->check_graphics_settings(center_pos, {radius*2, radius*2}) == gfx_chk_result::not_drawable) {
            rgl::add_frame_metrics_data_skipped_drawcalls(1);
            return;
        }

        if (thickness > 0) {
            rgl::shader_program_t pg = rocket::gl_get_shader(shader_id_t::circle_lines);
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

            auto vos = rgl::cache_compile_vo("circle");
            rgl::draw_shader(pg, vos.first, vos.second);
            return;
        }

        this->draw_rectangle({ center_pos, { radius * 2, radius * 2 } }, color, 0, 1);
    }

    void opengl_renderer_2d::draw_polygon(rocket::vec2f_t pos, float radius, rocket::rgba_color color, int segments, float rotation) {
        rocket::vec2f_t center_pos = {
            .x = pos.x - radius,
            .y = pos.y - radius
        };
        if (this->check_graphics_settings(center_pos, {radius*2, radius*2}) == gfx_chk_result::not_drawable) {
            rgl::add_frame_metrics_data_skipped_drawcalls(1);
            return;
        }

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
        rgl::shader_program_t pg = rocket::gl_get_shader(shader_id_t::polygon);

        auto color_nm = color.normalize();
        rgl::shader_location_t color_loc = rgl::get_shader_location(pg, "uColor");

        glUseProgram(pg);
        glUniform4f(color_loc, color_nm.x, color_nm.y, color_nm.z, color_nm.w);

        glBindVertexArray(vo.first);
        glUseProgram(pg);
        rgl::gl_draw_arrays(GL_TRIANGLE_FAN, 0, vertex_count);

        glDeleteVertexArrays(1, &vo.first);
        glDeleteBuffers(1, &vo.second);
    }

    void opengl_renderer_2d::draw_pixel(rocket::vec2f_t pos, rocket::rgba_color color) {
        if (this->check_graphics_settings(pos, {1,1}) == gfx_chk_result::not_drawable) {
            rgl::add_frame_metrics_data_skipped_drawcalls(1);
            return;
        }
        this->draw_rectangle({ pos, { 1, 1 } }, color, 0, 0, 0);
    }

    void opengl_renderer_2d::draw_rectangle(rocket::vec2f_t pos, rocket::vec2f_t size, rocket::rgba_color color, float rotation, float roundedness, bool lines) {
        if (this->check_graphics_settings(pos, size) == gfx_chk_result::not_drawable) {
            rgl::add_frame_metrics_data_skipped_drawcalls(1);
            return;
        }
        rocket::fbounding_box box = {pos, size};
        this->draw_rectangle(box, color, rotation, roundedness, lines);
    }

    void opengl_renderer_2d::begin_frame() {
        this->frame_started = true;
        frame_start_time = clock::now();
        delta_time = std::chrono::duration<double>(frame_start_time - last_time).count();
        last_time = frame_start_time;
    }

    void opengl_renderer_2d::show_splash() {
        static auto cli_args = util::get_clistate();
        if (cli_args.nosplash) {
            this->splash_shown = true;
            return;
        }
        this->splash_shown = true;

        float duration = 16384;

        auto tween = tweeny::from(0).to(0).during(duration / 2).via(tweeny::easing::cubicOut);

        std::vector<uint8_t> splash_screen = std::vector<uint8_t>(splash_screen_png, splash_screen_png + splash_screen_png_len);
        auto tx = std::make_shared<texture_t>();
        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc *img_data = stbi_load_from_memory(
            splash_screen.data(),
            static_cast<int>(splash_screen.size()),
            &width,
            &height,
            &channels,
            4
        );
        if (img_data == nullptr) {
            rocket::log("failed to load embedded splash texture", "opengl_renderer_2d", "show_splash", "error");
            return;
        }
        tx->size = { width, height };
        tx->channels = 4;
        tx->data.assign(img_data, img_data + (width * height * 4));
        stbi_image_free(img_data);

        bool final = false;

        bool splash_finished = false;

        while (window->is_running() && !splash_finished) {
            this->begin_frame();
            this->clear(rgba_color::black());
            {
                float alpha = tween.step((float) this->get_delta_time());
                float center_x = get_viewport_size().x / 2 - window->get_size().y / 2.f;

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

    void opengl_renderer_2d::begin_render_mode(render_mode_t mode) {
        // if (mode == render_mode_t::camera && this->cam != nullptr) {
        //     vec2f_t viewport = get_viewport_size();
        //
        //     glm::mat4 projection = glm::ortho(0.0f, viewport.x, viewport.y, 0.0f, -1.0f, 1.0f);
        //
        //     glm::mat4 view = glm::mat4(1.0f);
        //     view = glm::translate(view, glm::vec3(-cam->offset.x, -cam->offset.y, 0.0f));
        //     view = glm::rotate(view, glm::radians(-cam->rotation), glm::vec3(0,0,1));
        //     view = glm::scale(view, glm::vec3(cam->zoom, cam->zoom, 1.0f));
        //
        //     this->impl->camera_transform = projection * view;
        // }

        active_render_modes.push_back(mode);
    }

    render_cache_t* opengl_renderer_2d::create_render_cache(std::function<void(renderer_2d_i*)> cb) {
        r_assert(false && "TODO IMPLEMENT CROSS API RENDER CACHE");
        auto c = std::make_unique<render_cache_t>();

        // // c->fbo = rgl::create_fbo();
        // c->draw = cb;
        //
        // begin_render_cache(c.get());
        // auto off = this->override_viewport_offset;
        // auto sz = this->override_viewport_size;
        // if (this->override_viewport_offset == rocket::vec2f_t(-1, -1)) {
        //     off = {0,0};
        // }
        // if (this->override_viewport_size == rocket::vec2f_t(-1, -1)) {
        //     sz = rgl::get_viewport_size();
        // }
        // glViewport(off.x, off.y, sz.x, sz.y);
        // glClearColor(0,0,0,0);
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // cb(this);
        // end_render_cache(c.get());
        //
        // this->impl->render_caches.emplace_back(std::move(c));
        //
        // return this->impl->render_caches.back().get();
    }

    void opengl_renderer_2d::invalidate_render_cache(render_cache_t *c) {
        r_assert(false && "TODO IMPLEMENT CROSS API RENDER CACHE");
        r_assert(c != nullptr);
        // rgl::delete_fbo(c->fbo);
        // c->fbo = rgl::create_fbo();
        // bool _frame_started = this->frame_started;
        // this->frame_started = true;
        // begin_render_cache(c);
        // glClearColor(0,0,0,0);
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // c->draw(this);
        // end_render_cache(c);
        // this->frame_started = _frame_started;
    }

    void opengl_renderer_2d::begin_render_cache(render_cache_t *c) {
        r_assert(c != nullptr);
        // rgl::use_fbo(c->fbo);
        this->impl->render_cache_use_stack.push(c);
    }

    void opengl_renderer_2d::draw_render_cache(render_cache_t *c, rocket::vec2f_t pos, rocket::vec2f_t sz) {
        r_assert(false && "TODO IMPLEMENT CROSS API RENDER CACHE");
        if (this->check_graphics_settings(pos, sz) == gfx_chk_result::not_drawable) {
            rgl::add_frame_metrics_data_skipped_drawcalls(1);
            return;
        }
        r_assert(c != nullptr);
        // r_assert(rgl::get_active_fbo() != c->fbo);

        rgl::shader_program_t pg = rgl::get_paramaterized_textured_quad(pos, sz, 0, 0);
        rgl::texture_unit_handle_t unit;
        rgl::alloc_texture_unit(unit);
        glActiveTexture(unit.unit);
        // glBindTexture(GL_TEXTURE_2D, c->fbo.color_tex);
        glUniform1i(glGetUniformLocation(pg, "u_texture"), unit.unit - GL_TEXTURE0);
        glUniform1f(glGetUniformLocation(pg, "u_flip_y"), 1.f);
        rgl::draw_shader(pg, rgl::shader_use_t::textured_rect);
        rgl::free_texture_unit(unit);
    }
    
    void opengl_renderer_2d::draw_render_cache(render_cache_t *c, rocket::fbounding_box bbox) {
        r_assert(false && "TODO IMPLEMENT CROSS API RENDER CACHE");
        this->draw_render_cache(c, bbox.pos, bbox.size);
    }

    void opengl_renderer_2d::end_render_cache(render_cache_t *c) {
        r_assert(false && "TODO IMPLEMENT CROSS API RENDER CACHE");
        r_assert(c != nullptr);
        r_assert(!this->impl->render_cache_use_stack.empty());
        r_assert(this->impl->render_cache_use_stack.top() == c);

        this->impl->render_cache_use_stack.pop();

        if (this->impl->render_cache_use_stack.empty()) {
            rgl::reset_to_default_fbo();
        } else {
            // rgl::use_fbo(this->impl->render_cache_use_stack.top()->fbo);
        }
    }

    void opengl_renderer_2d::destroy_render_cache(render_cache_t *&c) {
        r_assert(false && "TODO IMPLEMENT CROSS API RENDER CACHE");
        r_assert(c != nullptr);
        for (auto it = this->impl->render_caches.begin(); it != this->impl->render_caches.end(); ++it) {
            if (it->get() == c) {
                this->impl->render_caches.erase(it);
                // if (rgl::get_active_fbo() == c->fbo) {
                //     rgl::reset_to_default_fbo();
                // }
                // rgl::delete_fbo(c->fbo);
                c = nullptr;
                break;
            }
        }
    }

    void opengl_renderer_2d::clear(rocket::rgba_color color) {
        this_frame_clear_color = color;

        vec4f_t clr = color.normalize();
        glClearColor(clr.x, clr.y, clr.z, clr.w);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // you probably want plugins to start rendering AFTER clearing
        if (flags.share_renderer_as_global) {
            __rallframestart();
        }
    }

    void opengl_renderer_2d::make_ready_texture(std::shared_ptr<rocket::texture_t> texture) {
        r_assert(texture != nullptr);
        if (texture->hdl == 0) {
            gl_object_t texture_object;
            texture_object.type = gl_object_type_t::texture;
            glGenTextures(1, &texture_object.value);
            glBindTexture(GL_TEXTURE_2D, texture_object.value);
            texture->hdl = ++this->impl->current_object_handle;
            this->bk_impl->objects[texture->hdl] = texture_object;

            // Use RGBA8 instead of SRGB8_ALPHA8 for universal compatibility
            #ifdef ROCKETGE__Platform_Android
                GLint internal_fmt = GL_RGBA8;
            #else
                GLint internal_fmt = GL_SRGB8_ALPHA8;
            #endif

#ifdef ROCKETGE__Platform_Android
            std::vector<uint8_t> rgba_data;
            const uint8_t* upload_data = texture->data.data();

            if (texture->channels == 3) {
                rgba_data.resize(texture->size.x * texture->size.y * 4);
                for (size_t i = 0; i < (size_t)(texture->size.x * texture->size.y); i++) {
                    rgba_data[i*4+0] = texture->data[i*3+0];
                    rgba_data[i*4+1] = texture->data[i*3+1];
                    rgba_data[i*4+2] = texture->data[i*3+2];
                    rgba_data[i*4+3] = 255;
                }
                upload_data = rgba_data.data();
            }

            glTexImage2D(GL_TEXTURE_2D, 0, internal_fmt,
                         texture->size.x, texture->size.y, 0,
                         GL_RGBA,
                         GL_UNSIGNED_BYTE, texture->channels == 3 ? rgba_data.data() : texture->data.data());
#else
            glTexImage2D(GL_TEXTURE_2D, 0, internal_fmt,
                         texture->size.x, texture->size.y, 0,
                         texture->channels == 4 ? GL_RGBA : GL_RGB,
                         GL_UNSIGNED_BYTE, texture->data.data());
#endif

            if (std::find(this->active_render_modes.begin(), this->active_render_modes.end(), render_mode_t::texture_filter_none) != this->active_render_modes.end()) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            } else {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
        }
    }

    void opengl_renderer_2d::draw_texture(std::shared_ptr<rocket::texture_t> texture, rocket::fbounding_box rect, float rotation, float roundedness) {
        if (this->check_graphics_settings(rect.pos, rect.size) == gfx_chk_result::not_drawable) {
            rgl::add_frame_metrics_data_skipped_drawcalls(1);
            return;
        }

        r_assert(texture != nullptr);

        rgl::shader_program_t pg = rgl::get_paramaterized_textured_quad(rect.pos, rect.size, rotation, roundedness);
        rgl::texture_unit_handle_t unit;
        rgl::alloc_texture_unit(unit);
        glActiveTexture(unit.unit);
        this->make_ready_texture(texture);
        gl_object_t obj = this->bk_impl->objects[texture->hdl];
        glBindTexture(GL_TEXTURE_2D, obj.value);
        glUniform1i(glGetUniformLocation(pg, "u_texture"), unit.unit - GL_TEXTURE0);
        rgl::draw_shader(pg, rgl::shader_use_t::textured_rect);
        rgl::free_texture_unit(unit);
    }

    void opengl_renderer_2d::draw_atlas_texture(
        std::shared_ptr<rocket::texture_t> atlas,
        rocket::fbounding_box rect,              // quad position & size on screen
        rocket::vec2f_t sprite_pos_in_atlas,     // top-left pixel of sprite inside atlas
        rocket::vec2f_t sprite_size_in_atlas,    // width/height of the sprite inside atlas
        float rotation,
        float roundedness
    ) {
        if (this->check_graphics_settings(rect.pos, rect.size) == gfx_chk_result::not_drawable) {
            rgl::add_frame_metrics_data_skipped_drawcalls(1);
            return;
        }

        r_assert(atlas != nullptr);

        rgl::shader_program_t pg = rocket::gl_get_shader(shader_id_t::atlas_textured_rectangle);
        rocket::vec2f_t viewport_size = this->get_viewport_size();

        glm::mat4 projection = glm::ortho(0.f, viewport_size.x, viewport_size.y, 0.f, -1.f, 1.f);

        rocket::vec2f_t size = rect.size;
        rocket::vec2f_t pos  = rect.pos;
        float cx = pos.x + size.x * 0.5f;
        float cy = pos.y + size.y * 0.5f;

        glm::mat4 transform = projection
            * glm::translate(glm::mat4(1.0f), glm::vec3(cx, cy, 0.0f))
            * glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f))
            * glm::translate(glm::mat4(1.0f), glm::vec3(-size.x * 0.5f, -size.y * 0.5f, 0.0f))
            * glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));

        glUseProgram(pg);
        glUniformMatrix4fv(glGetUniformLocation(pg, "u_transform"), 1, GL_FALSE, glm::value_ptr(transform));
        glUniform2f(glGetUniformLocation(pg, "u_size"), size.x, size.y);
        glUniform1f(glGetUniformLocation(pg, "u_radius"), roundedness);

        rgl::texture_unit_handle_t unit;
        rgl::alloc_texture_unit(unit);
        glActiveTexture(unit.unit);
        this->make_ready_texture(atlas);
        gl_object_t obj = this->bk_impl->objects[atlas->hdl];
        glBindTexture(GL_TEXTURE_2D, obj.value);
        glUniform1i(glGetUniformLocation(pg, "u_texture"), unit.unit - GL_TEXTURE0);

        rocket::vec2f_t atlas_size{ 1.f * atlas->size.x, 1.f * atlas->size.y };
        rocket::vec2f_t uv_tex_pos  = sprite_pos_in_atlas / atlas_size;
        rocket::vec2f_t uv_tex_size = sprite_size_in_atlas / atlas_size;

        glUniform2f(glGetUniformLocation(pg, "u_texPos"), uv_tex_pos.x, uv_tex_pos.y);
        glUniform2f(glGetUniformLocation(pg, "u_texSize"), uv_tex_size.x, uv_tex_size.y);

        static const auto vos = rgl::cache_compile_vo("atlas_texture");
        if (!vos.first || !vos.second) std::terminate();
        rgl::draw_shader(pg, vos.first, vos.second);
        rgl::free_texture_unit(unit);
    }

    void opengl_renderer_2d::draw_rectangle(rocket::fbounding_box rect, rocket::rgba_color color, float rotation, float roundedness, bool lines) {
        if (this->check_graphics_settings(rect.pos, rect.size) == gfx_chk_result::not_drawable) {
            rgl::add_frame_metrics_data_skipped_drawcalls(1);
            return;
        }
        rgl::shader_program_t pg = rgl::get_paramaterized_quad(rect.pos, rect.size, color, rotation, roundedness);
        if (lines) {
            rocket::vec2f_t pos = rect.pos;
            rocket::vec2f_t size = rect.size;
            constexpr float thickness = 1.f;
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
        
        rgl::draw_shader(pg, rgl::shader_use_t::rect);
    }
   
    void opengl_renderer_2d::draw_text(const rocket::text_t& text_, rocket::vec2f_t position) {
        rocket::text_t text = text_;
        if (check_graphics_settings(position, text.measure()) == gfx_chk_result::not_drawable) {
            rgl::add_frame_metrics_data_skipped_drawcalls(text.text.size());
            return;
        }
        static auto cli_args = util::get_clistate();
        if (cli_args.notext) return;

        static rgl::shader_program_t shader_program = rgl::get_shader(rgl::shader_use_t::text);
        glUseProgram(shader_program);

        static const rgl::shader_location_t color_loc = glGetUniformLocation(shader_program, "u_color");
        static const rgl::shader_location_t tex_loc   = glGetUniformLocation(shader_program, "u_texture");

        glUniform3f(color_loc,
            text.color.x / 255.0f,
            text.color.y / 255.0f,
            text.color.z / 255.0f);
        glUniform1i(tex_loc, 0);

        float screen_w = (float)window->get_size().x;
        float screen_h = (float)window->get_size().y;

        stbtt_fontinfo info;
        stbtt_InitFont(&info, text.font->ttf_data.data(), 0);
        int ascent;
        stbtt_GetFontVMetrics(&info, &ascent, nullptr, nullptr);
        float scale    = stbtt_ScaleForPixelHeight(&info, text.size);
        float baseline = ascent * scale;

        float x = position.x;
        float y = position.y + baseline;

        std::vector<float> verts;
        verts.reserve(text.text.size() * 6 * 4);

        for (char c : text.text) {
            if (c < 32) continue;
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(text.font->cdata->a, 512, 512, c - 32, &x, &y, &q, 1);

            float x0 = (q.x0 / screen_w) * 2.0f - 1.0f;
            float y0 = 1.0f - (q.y0 / screen_h) * 2.0f;
            float x1 = (q.x1 / screen_w) * 2.0f - 1.0f;
            float y1 = 1.0f - (q.y1 / screen_h) * 2.0f;

            float vq[6][4] = {
                { x0, y0, q.s0, q.t0 },
                { x0, y1, q.s0, q.t1 },
                { x1, y1, q.s1, q.t1 },
                { x0, y0, q.s0, q.t0 },
                { x1, y1, q.s1, q.t1 },
                { x1, y0, q.s1, q.t0 },
            };
            for (auto& v : vq)
                verts.insert(verts.end(), std::begin(v), std::end(v));
        }

        if (verts.empty()) return;

        rgl::texture_unit_handle_t unit;
        rgl::alloc_texture_unit(unit);
        glActiveTexture(unit.unit);
        glBindTexture(GL_TEXTURE_2D, this->bk_impl->objects[text.font->hdl].value);

        auto text_vo = rgl::get_text_vos();
        glBindVertexArray(text_vo.first);
        glBindBuffer(GL_ARRAY_BUFFER, text_vo.second);

        glBufferData(GL_ARRAY_BUFFER,
                     verts.size() * sizeof(float),
                     verts.data(),
                     GL_DYNAMIC_DRAW);

        rgl::gl_draw_arrays(GL_TRIANGLES, 0, (GLsizei)(verts.size() / 4));

        rgl::free_texture_unit(unit);
    }

    void opengl_renderer_2d::draw_shader(const shader_i &abs_shader) {
        const opengl_shader_t *shader = dynamic_cast<const opengl_shader_t*>(&abs_shader);
        if (this->check_graphics_settings({-1,-1}, {-1,-1}) == gfx_chk_result::not_drawable) {
            rgl::add_frame_metrics_data_skipped_drawcalls(1);
            return;
        }
        rgl::draw_shader(shader->glprogram, shader->vao, shader->vbo);
    }

    // void opengl_renderer_2d::draw_fbo(rgl::fbo_t fbo, vec2f_t pos, vec2f_t size) {
    //     r_assert(false && "TODO IMPLEMENT CROSS API FBO");
    //     // auto prev = rgl::is_active_any_fbo() ? rgl::get_active_fbo() : rGL_FBO_INVALID;
    //
    //     // if (prev == fbo) {
    //     //     rgl::reset_to_default_fbo();
    //     // }
    //
    //     rgl::texture_unit_handle_t unit;
    //     rgl::alloc_texture_unit(unit);
    //
    //     rgl::shader_program_t pg = rgl::get_paramaterized_textured_quad(pos, size, 0, 0);
    //     rgl::bind_texture_unit(unit.unit);
    //     rgl::bind_texture(fbo.color_tex);
    //     int u_texture = rgl::get_shader_location(pg, "u_texture");
    //     int u_flip_y = rgl::get_shader_location(pg, "u_flip_y");
    //     rgl::gl_uniform1i(pg, u_texture, unit.unit - rgl::gl_texture0);
    //     rgl::gl_uniform1f(pg, u_flip_y, 1.f);
    //     rgl::draw_shader(pg, rgl::shader_use_t::textured_rect);
    //     rgl::free_texture_unit(unit);
    //
    //     // if (prev != rGL_FBO_INVALID) {
    //     //     rgl::use_fbo(prev);
    //     // } else {
    //     //     rgl::reset_to_default_fbo();
    //     // }
    // }
    //
    // void opengl_renderer_2d::draw_fbo(rgl::fbo_t fbo, const shader_i &shader) {
    //     auto prev = rgl::is_active_any_fbo() ? rgl::get_active_fbo() : rGL_FBO_INVALID;
    //
    //     if (prev == fbo) {
    //         rgl::reset_to_default_fbo();
    //     }
    //
    //     rgl::texture_unit_handle_t unit;
    //     rgl::alloc_texture_unit(unit);
    //
    //     rgl::bind_texture_unit(unit.unit);
    //     rgl::bind_texture(fbo.color_tex);
    //
    //     int u_texture = rgl::get_shader_location(shader.glprogram, "u_texture");
    //     if (u_texture != -1) {
    //         shader.set_uniform("u_texture", static_cast<int>(unit.unit - rgl::gl_texture0));
    //     }
    //
    //     this->draw_shader(shader);
    //
    //     rgl::free_texture_unit(unit);
    //
    //     if (prev != rGL_FBO_INVALID) {
    //         rgl::use_fbo(prev);
    //     } else {
    //         rgl::reset_to_default_fbo();
    //     }
    // }

    void opengl_renderer_2d::set_wireframe(bool x) {
        this->wireframe = x;
#ifdef ROCKETGE__Platform_Desktop
        glPolygonMode(GL_FRONT_AND_BACK, x ? GL_LINE : GL_FILL);
#else
        rocket::log("Wireframes are not supported on this platform", "opengl_renderer_2d", "set_wireframe", "error");
#endif
    }

    void opengl_renderer_2d::set_vsync(bool x) {
        this->vsync = x;
        this->window->set_vsync(x);
    }

    void opengl_renderer_2d::set_fps(int fps) {
        this->fps = fps;
    }

    void opengl_renderer_2d::set_graphics_settings(graphics_settings_t settings) {
        this->graphics_settings = settings;
    }

    void opengl_renderer_2d::set_viewport_size(vec2f_t size) {
        this->override_viewport_size = size;
    }

    void opengl_renderer_2d::set_viewport_offset(vec2f_t offset) {
        this->override_viewport_offset = offset;
    }

    void opengl_renderer_2d::set_camera(camera_2d *cam) {
        rocket::log("cameras are not implemented", "opengl_renderer_2d", "set_camera", "fixme");
        this->cam = cam;
    }

    void opengl_renderer_2d::draw_fps(vec2f_t pos) {
        std::string fps_text = "FPS: " + std::to_string(static_cast<int>(std::round(get_current_fps())));

        rocket::text_t fps = rocket::text_t(fps_text, 24, rocket::rgb_color::green());
        this->draw_text(fps, pos);
    }
}

namespace std {
    template<>
    struct hash<std::pair<float, float>> {
        size_t operator()(const std::pair<float, float> &p) const {
            return std::hash<float>()(p.first) ^ std::hash<float>()(p.second);
        }
    };
}

namespace rocket {
    std::vector<rgba_color> opengl_renderer_2d::get_framebuffer() {
        static std::unordered_map<std::pair<float, float>, std::vector<rgba_color>> fb_pixels;
        std::pair<float, float> key = { rgl::get_viewport_size().x, rgl::get_viewport_size().y };
        auto it = fb_pixels.find(key);
        if (it != fb_pixels.end()) {
            return it->second;
        }

        fb_pixels[key] = std::vector<rgba_color>(rgl::get_viewport_size().x * rgl::get_viewport_size().y);
        return fb_pixels[key];
    }

    void opengl_renderer_2d::push_framebuffer(const std::vector<rgba_color> &framebuffer) {
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

        rgl::texture_unit_handle_t unit;
        rgl::alloc_texture_unit(unit);
        glActiveTexture(unit.unit);
        glBindTexture(GL_TEXTURE_2D, framebuffer_tx);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, rgl::get_viewport_size().x, rgl::get_viewport_size().y, GL_RGBA, GL_UNSIGNED_BYTE, flat.data());

        static rgl::shader_program_t shader = rgl::get_paramaterized_textured_quad({0,0}, rgl::get_viewport_size(), 0.f, 0.f);
        glUniform1i(glGetUniformLocation(shader, "u_texture"), unit.unit - GL_TEXTURE0);
        rgl::draw_shader(shader, rgl::shader_use_t::textured_rect);
        rgl::free_texture_unit(unit);
    }

    vec2f_t opengl_renderer_2d::get_viewport_size() {
        return rgl::get_viewport_size();
    }

    static void draw_fullscreen_quad() {
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

    void opengl_renderer_2d::end_render_mode(render_mode_t mode) {
        if (mode == render_mode_t::camera) {
            this->impl->camera_transform = glm::mat4(1.0f);
        }

        for (auto it = this->active_render_modes.begin(); it != this->active_render_modes.end(); it++) {
            if (*it == mode) {
                this->active_render_modes.erase(it);
                break;
            }
        }
    }

    bool opengl_renderer_2d::has_frame_began() {
        return this->frame_started;
    }

    bool opengl_renderer_2d::has_frame_ended() {
        return !this->frame_started;
    }

    void opengl_renderer_2d::end_frame() {
        if (flags.share_renderer_as_global) {
            __rallframeend();
        }
        if (util::get_clistate().debugoverlay)
            util::draw_debug_overlay(this);
        this->frame_started = false;
        auto frame_end_time = clock::now();
        this->window->swap_buffers();
        rgl::frame_metrics_t fmetrics = rgl::get_frame_metrics();
        int drawcalls = fmetrics.drawcalls;
        if (drawcalls > rGL_MAX_RECOMMENDED_DRAWCALLS) {
            rocket::log("Too many drawcalls! (" + std::to_string(drawcalls) + ") Frames may suffer", "opengl_renderer_2d", "end_frame", "warning");
        }
        int tricount = fmetrics.tricount;
        if (tricount > rGL_MAX_RECOMMENDED_TRICOUNT) {
            rocket::log("Too many triangles! (" + std::to_string(tricount) + ") Frames may suffer", "opengl_renderer_2d", "end_frame", "warning");
        }

        rgl::reset_frame_metrics();

        rocket::vec2f_t final_viewport_position = {  0,  0 };
        rocket::vec2f_t final_viewport_size     = { -1, -1 };

        static rocket::vec2f_t last_frame_vp_size = final_viewport_size;

        // If override size is set, use that, otherwise fallback to window size
        if (this->override_viewport_size != rocket::vec2f_t{ -1, -1 }) {
            final_viewport_size = this->override_viewport_size;
        } else {
            final_viewport_size = {
                static_cast<float>(this->window->size.x),
                static_cast<float>(this->window->size.y)
            };
        }

        if (last_frame_vp_size != final_viewport_size) {
            for (auto &cache : this->impl->render_caches) {
                this->invalidate_render_cache(cache.get());
            }
        }
        last_frame_vp_size = final_viewport_size;

        // If override offset is set, use that
        if (this->override_viewport_offset != rocket::vec2f_t{ -1, -1 }) {
            final_viewport_position = this->override_viewport_offset;
        }

        static auto cli_args = util::get_clistate();

        if (!cli_args.viewport_size_set) {
            rgl::update_viewport(final_viewport_position, final_viewport_size);
        }
        rgl::fbo_t fbo = rgl::get_active_fbo();
        rgl::reset_to_default_fbo();

        rgl::update_viewport(final_viewport_position, final_viewport_size);

        if (fbo != rGL_FBO_INVALID) {
            rgl::use_fbo(fbo);
            if (!cli_args.viewport_size_set) {
                rgl::update_viewport(final_viewport_position, final_viewport_size);
            }
        }

        frame_counter++;

        if (this->fps == rocket::fps_uncapped) {
            delta_time = std::chrono::duration<double>(frame_start_time - last_time).count();
            return;
        }

        if (this->vsync) {
            delta_time = std::chrono::duration<double>(frame_start_time - last_time).count();
            return; // We're done here
        }

        rgl::run_all_scheduled_gl();

        double frame_duration = std::chrono::duration<double>(frame_end_time - frame_start_time).count();

        if (fps == 0) {
            rocket::log("Target FPS 0 is too low", "opengl_renderer_2d", "end_frame", "fatal");
            rocket::exit(1);
        }

        double frametime_limit = 1.0 / (fps + 0);

        // Perfect Frame Timer
        util::frame_timer_wait_for(frame_duration, frametime_limit, cli_args.software_frame_timer, this->fps, this->frame_start_time);

        rgl::update_draw_metrics_data(frame_duration + std::chrono::duration<double>(clock::now() - frame_end_time).count(), 1 / this->delta_time);

        delta_time = std::chrono::duration<double>(frame_start_time - last_time).count();
    }

    struct batched_quad_t {
        vec2f_t pos = {0,0};
        vec2f_t size = {0,0};
        vec4f_t color = { 0,0,0,0 };
        GLuint gltxid;
    };

    double opengl_renderer_2d::get_delta_time() {
        return delta_time;
    }

    int opengl_renderer_2d::get_fps() {
        return fps;
    }

    uint64_t opengl_renderer_2d::get_framecount() {
        return this->frame_counter;
    }

    bool opengl_renderer_2d::get_vsync() {
        return vsync;
    }

    bool opengl_renderer_2d::get_wireframe() {
        return wireframe;
    }

    float opengl_renderer_2d::get_current_fps() {
        return rgl::get_draw_metrics().avg_fps;
    }

    int opengl_renderer_2d::get_drawcalls() {
        return rgl::get_frame_metrics().drawcalls;
    }

    rgl::draw_metrics_t opengl_renderer_2d::get_draw_metrics() {
        return rgl::get_draw_metrics();
    }

    const graphics_settings_t &opengl_renderer_2d::get_graphics_settings() {
        return this->graphics_settings;
    }

    api_object_t opengl_renderer_2d::get_framebuffer_texture() {
        r_assert(false && "TODO IMPL GET FRAMEBUFFER TEXTURE API OBJ");
        return 0;
        // rgl::scoped_gl_texture_t t;
        // glBindTexture(GL_TEXTURE_2D, t.id);
        //
        // int w = rgl::get_viewport_size().x;
        // int h = rgl::get_viewport_size().y;
        //
        // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        //
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        //
        // glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, w, h);
        //
        // return t; // Implicit move
    }

    camera_2d* opengl_renderer_2d::get_camera() {
        rocket::log("cameras are not implemented", "opengl_renderer_2d", "get_camera", "fixme");
        return this->cam;
    }

    glm::mat4 opengl_renderer_2d::get_camera_matrix() {
        return {}; // FIXME return 
    };

    void opengl_renderer_2d::begin_scissor_mode(rocket::fbounding_box rect) {
        glEnable(GL_SCISSOR_TEST);

        glScissor(
            rect.pos.x,
            window->get_size().y - rect.pos.y - rect.size.y,
            rect.size.x,
            rect.size.y
        );
    }

    void opengl_renderer_2d::begin_scissor_mode(rocket::vec2f_t pos, rocket::vec2f_t size) {
        this->begin_scissor_mode({ pos, size });
    }

    void opengl_renderer_2d::begin_scissor_mode(float x, float y, float sx, float sy) {
        this->begin_scissor_mode({ { x, y }, { sx, sy } });
    }

    void opengl_renderer_2d::end_scissor_mode() {
        glDisable(GL_SCISSOR_TEST);
    }

    void opengl_renderer_2d::close() {
        if (window == nullptr) return;
        if (window->wbi_impl != nullptr) {
            if (window->wbi_impl->bound_renderer2d == this) {
                window->wbi_impl->bound_renderer2d = nullptr;
            }
        } else {
            rocket::log("window destructor ran before opengl_renderer_2d destructor, incorrect order of deletion",
                        "opengl_renderer_2d", "close", "warn");
        }

        window = nullptr; // unbind window

        if (util::get_global_renderer_2d() == this) {
            util::set_global_renderer_2d(nullptr);
        }

        rgl::cleanup_all();
        rgl::reset();
        shader_provider_reset();
        delete this->impl;
        this->impl = nullptr;

        delete this->bk_impl;
        this->bk_impl = nullptr;
        util::glinit(false);
    }

    opengl_renderer_2d::~opengl_renderer_2d() {
        close();
    }
}
