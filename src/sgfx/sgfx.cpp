#include <lib/glad/glad.h>
#include <atomic>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/matrix.hpp>
#include <iostream>
#include <mutex>
#include <sgfx/sgfx.hpp>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <thread>
#include <intl_macros.hpp>

namespace sgfx::gl {
    void destroy_object(sgfx::context_t &ctx, gpu_object_t handle) {
        gl::object_t obj = ctx.gl_objects.at(handle);
        if (obj.type == gl::obj_type_e::context) {

        } else if (obj.type == gl::obj_type_e::draw_data) {
            gl::draw_data_t draw_data = std::get<gl::draw_data_t>(obj.data);
            glDeleteVertexArrays(1, &draw_data.vao);
            glDeleteBuffers(1, &draw_data.vbo);
            glDeleteBuffers(1, &draw_data.ebo);
        } else if (obj.type == gl::obj_type_e::shader) {
            gl::shader_t shader = std::get<gl::shader_t>(obj.data);
            glDeleteProgram(shader);
        }
    }
}

namespace sgfx::vk {
    void destroy_object(const sgfx::context_t &, gpu_object_t obj) {
        throw std::runtime_error("Cleanup not supported");
    }
}

namespace sgfx::internal {
    //
    // std::unordered_map<gpu_object_t, gl::object_t> gl_objects;
    // std::unordered_map<gpu_object_t, vk::object_t> vk_objects;
    // std::unordered_map<unique_object_id_t, compiled_object_t> unique_objects;

    bool is_object_vulkan(const context_t &ctx, gpu_object_t obj) {
        return ctx.vk_objects.find(obj) != ctx.vk_objects.end();
    }

    bool is_object_opengl(const context_t &ctx, gpu_object_t obj) {
        return ctx.gl_objects.find(obj) != ctx.gl_objects.end();
    }
}

namespace callback {
    static void gl_debug(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei, const GLchar* message, const void*) {
        std::string srcStr;
        switch (source) {
            case GL_DEBUG_SOURCE_API:             srcStr = "API"; break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   srcStr = "Window System"; break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER: srcStr = "Shader Compiler"; break;
            case GL_DEBUG_SOURCE_THIRD_PARTY:     srcStr = "Third Party"; break;
            case GL_DEBUG_SOURCE_APPLICATION:     srcStr = "Application"; break;
            case GL_DEBUG_SOURCE_OTHER:           srcStr = "Other"; break;
        }

        std::string typeStr;
        switch (type) {
            case GL_DEBUG_TYPE_ERROR:               typeStr = "Error"; break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "Deprecated Behavior"; break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  typeStr = "Undefined Behavior"; break;
            case GL_DEBUG_TYPE_PORTABILITY:         typeStr = "Portability"; break;
            case GL_DEBUG_TYPE_PERFORMANCE:         typeStr = "Performance"; break;
            case GL_DEBUG_TYPE_MARKER:              typeStr = "Marker"; break;
            case GL_DEBUG_TYPE_PUSH_GROUP:          typeStr = "Push Group"; break;
            case GL_DEBUG_TYPE_POP_GROUP:           typeStr = "Pop Group"; break;
            case GL_DEBUG_TYPE_OTHER:               typeStr = "Other"; break;
        }

        std::string sevStr;
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:         sevStr = "High"; break;
            case GL_DEBUG_SEVERITY_MEDIUM:       sevStr = "Medium"; break;
            case GL_DEBUG_SEVERITY_LOW:          sevStr = "Low"; break;
            case GL_DEBUG_SEVERITY_NOTIFICATION: sevStr = "Notification"; break;
        }

        std::vector<std::string> log_messages = {
            "Error Caught at:",
            "   Type: " + typeStr,
            "   Severity: " + sevStr,
            "   ID: " + std::to_string(id),
            "   Message: " + std::string(message),
            "   Source: " + srcStr,
            "Dumping GL state:"
        };

        for (const auto &l : log_messages) {
            std::cout << l << '\n';
        }
    }
}

namespace sgfx {
    uint32_t allocate_id() {
        static uint32_t accum = 0;
        return ++accum;
    }

    sgfx::context_t create_context(const sgfx::context_options_t &options) {
        sgfx::context_t ctx;
        ctx.id = allocate_id();

        glViewport(0, 0, 1, 1);

        if (options.multisampling)
            glEnable(GL_MULTISAMPLE);

        if (options.preferred_colorspace == colorspace_e::srgb)
            glEnable(GL_FRAMEBUFFER_SRGB);

        if (options.depth_buffer) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (options.debug) {
            int flags;
            glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
            if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
                glEnable(GL_DEBUG_OUTPUT);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                glDebugMessageControl(
                    GL_DONT_CARE,  // source
                    GL_DONT_CARE,  // type
                    GL_DEBUG_SEVERITY_LOW,  // severity to filter
                    0,             // count of ids to filter
                    nullptr,       // ids array
                    GL_TRUE       // GL_TRUE to enable, GL_FALSE to disable
                );
                glDebugMessageCallback(callback::gl_debug, nullptr);
            }
        }

        ctx.gl_objects[ctx.id] = gl::object_t {
            gl::obj_type_e::context,
            gl::context_t {
                .options = options
            }
        };

        return ctx;
    }

    std::vector<std::string> get_context_creation_log_messages(
        backend_e bk,
        std::string driver_string,
        std::string api_version,
        int loaded_extensions,
        std::vector<std::pair<std::string, std::string>> features,
        bool debug,
        std::string gpu_name,
        std::string gpu_vendor
    ) noexcept {
        std::string api = "Null";

        if (bk == backend_e::opengl) {
            api = "OpenGL";
        } else if (bk == backend_e::vulkan) {
            api = "Vulkan";
        }

        return {
            "Renderer Information",
            "Driver: " + driver_string,
            "Version: " + api_version,
            std::to_string(loaded_extensions) + " extensions loaded",
            [&features]() -> std::string {
                std::string accum;
                for (const auto &[k, v] : features) {
                    accum += k + ": " + v + "\n";
                }

                return accum.size() > 0 
                    ? accum.substr(0, accum.size() - 1) 
                    : "No Extra Features";
            } (),
            "Debug: " + std::string(debug ? "[TRUE]" : "[FALSE]"),
            "GPU:",
            "  Name: " + gpu_name,
            "  Vendor: " + gpu_vendor
        };
    }

    void destroy_object(context_t &ctx, gpu_object_t obj) {
        if (internal::is_object_opengl(ctx, obj))
            gl::destroy_object(ctx, obj);
        else if (internal::is_object_vulkan(ctx, obj))
            vk::destroy_object(ctx, obj);
        else
            throw std::runtime_error("Invalid object");
    }

    void renderer_t::begin_frame() noexcept {
        
    }

    void renderer_t::enqueue(
        unique_object_id_t unique_object_id,
        const draw_object_t &object, 
        const transform_t &transform,
        const visual_effects_t &visual_effects,
        const draw_instructions_t &instructions,
        const std::vector<shader_parameter_t> &shader_params
    ) noexcept {
        this->drawcalls.emplace_back(unique_object_id, object, transform, visual_effects, instructions, shader_params);
    }

    void renderer_t::enqueue(const drawcall_t &dc) noexcept {
        this->drawcalls.push_back(dc);
    }

    void renderer_t::clear(rgba_color color) noexcept {
        vec4f_t nm = color.normalize();
        glClearColor(nm.x, nm.y, nm.z, nm.w);
        GLbitfield bits = GL_COLOR_BUFFER_BIT;
        if (std::get<gl::context_t>(this->ctx->gl_objects[this->ctx->id].data).options.depth_buffer)
            bits |= GL_DEPTH_BUFFER_BIT;
        glClear(bits);
    }

    gl::draw_data_t compile_draw_data(const std::vector<vertex_t> &vertices, const std::vector<uint32_t> &indices) {
        GLuint vao, vbo, ebo;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex_t), vertices.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, pos));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, uv));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);

        return { vao, vbo, ebo };
    }

    glm::mat4 build_model(const transform_t &t) {
        glm::mat4 model = glm::mat4(1.0f);

        // model = glm::translate(glm::mat4(1.0f), glm::vec3(cx, cy, 0.0f))
        //     * glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f))
        //     * glm::translate(glm::mat4(1.0f), glm::vec3(-size.x * 0.5f, -size.y * 0.5f, 0.0f))
        //     * glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));

        model = glm::translate(model, glm::vec3(t.position.x, t.position.y, t.position.z));
        model = glm::translate(model, glm::vec3(0.5f * t.scale.x, 0.5f * t.scale.y, 0.0f));
        model = glm::rotate(model, t.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(-0.5f * t.scale.x, -0.5f * t.scale.y, 0.0f));
        model = glm::scale(model, glm::vec3(t.scale.x, t.scale.y, t.scale.z));

        return model;
    }

    GLuint compile_shader(const char *vsrc, const char *fsrc, std::string *output_log) {
        GLuint glshaderv = glCreateShader(GL_VERTEX_SHADER);
        GLuint glshaderf = glCreateShader(GL_FRAGMENT_SHADER);
        GLuint glprogram = 0;
        while (glGetError() != GL_NO_ERROR) {}
        glShaderSource(glshaderv, 1, &vsrc, nullptr);
        glCompileShader(glshaderv);

        glShaderSource(glshaderf, 1, &fsrc, nullptr);
        glCompileShader(glshaderf);

        glprogram = glCreateProgram();
        glAttachShader(glprogram, glshaderv);

        glAttachShader(glprogram, glshaderf);

        glLinkProgram(glprogram);

        glDeleteShader(glshaderv);
        glDeleteShader(glshaderf);

        if (output_log != nullptr) {
            GLint success;
            glGetShaderiv(glprogram, GL_COMPILE_STATUS, &success);
            if (!success) {
                char infoLog[1024];
                glGetShaderInfoLog(glprogram, sizeof(infoLog), nullptr, infoLog);
                *output_log = infoLog;
                return 0;
            }
        }

        return glprogram;
    }

    void texture_t::upload_to_gpu(context_t &ctx) noexcept {
        this->tx = allocate_id();
        gl::texture_t tx = {};
        glGenTextures(1, &tx.tx);
        glBindTexture(GL_TEXTURE_2D, tx);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8,
             size.x, size.y, 0,
             channels == 4 ? GL_RGBA : GL_RGB,
             GL_UNSIGNED_BYTE, data.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);
        ctx.gl_objects[this->tx] = gl::object_t {
            .type = gl::obj_type_e::texture,
            .data = tx
        };
    }

    void texture_t::bind(const context_t &ctx) const noexcept {
        glBindTexture(GL_TEXTURE_2D, std::get<gl::texture_t>(ctx.gl_objects.at(this->tx).data).tx);
    }

    texture_t::~texture_t() {
        if (this->tx != 0) {
            // destroy_object(this->ctx-> this->tx);
        }
    }

    bool shader_t::compile(context_t &ctx, std::string *output_log) noexcept {
        GLuint shader = compile_shader(vertex_src.c_str(), fragment_src.c_str(), output_log);
        this->obj = allocate_id();
        ctx.gl_objects[obj] = gl::object_t {
            .type = gl::obj_type_e::shader,
            .data = gl::shader_t { shader }
        };

        return shader == 0 ? false : true;
    }

    void shader_t::gl_provide_precompiled(context_t &ctx, unsigned int gl_program) noexcept {
        this->obj = allocate_id();
        ctx.gl_objects[obj] = gl::object_t {
            .type = gl::obj_type_e::shader,
            .data = gl::shader_t { gl_program }
        };
    }

    shader_t shader_t::from(context_t &ctx, unsigned int gl_program) noexcept {
        shader_t s;
        s.gl_provide_precompiled(ctx, gl_program);
        return s;
    }

    void renderer_t::flush_queue() noexcept {
        for (const auto &obj : this->drawcalls) {
            gl::draw_data_t draw_data;
            glm::mat4 transform = this->ctx->proj * build_model(obj.transform);

            GLuint shader = 0;

            if (this->ctx->unique_objects.find(obj.id) == this->ctx->unique_objects.end()) {
                gpu_object_t draw_data_obj = allocate_id();
                draw_data = compile_draw_data(
                    obj.draw_obj.draw_data.vertices, 
                    obj.draw_obj.draw_data.indices
                );
                this->ctx->gl_objects[draw_data_obj] = gl::object_t {
                    .type = gl::obj_type_e::draw_data,
                    .data = draw_data
                };
                this->ctx->unique_objects[obj.id] = {
                    .draw_data = draw_data_obj,
                    .shader = obj.draw_obj.shader.obj
                };

                shader = std::get<gl::shader_t>(this->ctx->gl_objects[this->ctx->unique_objects[obj.id].shader].data);
            } else {
                shader = std::get<gl::shader_t>(this->ctx->gl_objects[this->ctx->unique_objects[obj.id].shader].data);
                draw_data = std::get<gl::draw_data_t>(this->ctx->gl_objects[this->ctx->unique_objects[obj.id].draw_data].data);
            }

            r_assert(shader != 0);

            glUseProgram(shader);

            for (const auto &param : obj.shader_params) {
                GLint loc = glGetUniformLocation(shader, param.key.c_str());
                if (loc != -1) {
                    std::visit([loc, &param](auto&& v) {
                        using T = std::decay_t<decltype(v)>;

                        if constexpr (std::is_same_v<T, std::monostate>) {
                        } else if constexpr (std::is_same_v<T, int>) {
                            glUniform1i(loc, std::get<int>(param.value));
                        } else if constexpr (std::is_same_v<T, float>) {
                            glUniform1f(loc, std::get<float>(param.value));
                        } else if constexpr (std::is_same_v<T, std::array<float, 2>>) {
                            auto arr = std::get<std::array<float, 2>>(param.value);
                            glUniform2f(loc, arr[0], arr[1]);
                        } else if constexpr (std::is_same_v<T, std::array<float, 3>>) {
                            auto arr = std::get<std::array<float, 3>>(param.value);
                            glUniform3f(loc, arr[0], arr[1], arr[2]);
                        } else if constexpr (std::is_same_v<T, std::array<float, 4>>) {
                            auto arr = std::get<std::array<float, 4>>(param.value);
                            glUniform4f(loc, arr[0], arr[1], arr[2], arr[3]);
                        } else if constexpr (std::is_same_v<T, std::array<std::array<float, 4>, 4>>) {
                            auto mat4 = std::get<std::array<std::array<float, 4>, 4>>(param.value);
                            glUniformMatrix4fv(loc, 1, GL_FALSE, &mat4[0][0]);
                        }
                    }, param.value);
                } else {
                    r_assert(loc == -1);
                }
            }

            GLint u_model = glGetUniformLocation(shader, "u_sgfx_model");
            GLint u_color = glGetUniformLocation(shader, "u_sgfx_color");

            if (u_model != -1)
                glUniformMatrix4fv(u_model, 1, GL_FALSE, &transform[0][0]);
            auto color_nm = obj.visual_effects.color.normalize();
            if (u_color != -1)
                glUniform4f(u_color, color_nm.x, color_nm.y, color_nm.z, color_nm.w);

            glBindVertexArray(draw_data.vao);

            if (obj._post_bind_vao) {
                obj._post_bind_vao(draw_data.vao);
            }

            glDrawElementsInstanced(
                GL_TRIANGLES, 
                obj.draw_obj.draw_data.indices.size(), 
                GL_UNSIGNED_INT, 
                nullptr, 
                1
            );
        }

        this->drawcalls.clear();
    }

    void renderer_t::end_frame() noexcept {
        flush_queue();

        glUseProgram(0);

        vec2i_t sz = framebuffer_size_poller();
        glViewport(0, 0, sz.x, sz.y);
        this->viewport = {
            { 0, 0 },
            { sz.x * 1.f, sz.y * 1.f }
        };
    }

    renderer_t::renderer_t(context_t *ctx, std::function<vec2i_t()> framebuffer_size_poller) {
        this->framebuffer_size_poller = framebuffer_size_poller;
        vec2i_t sz = framebuffer_size_poller();
        glViewport(0, 0, sz.x, sz.y);
        this->viewport = {
            { 0, 0 },
            { sz.x * 1.f, sz.y * 1.f }
        };
        this->ctx = ctx;
    }

    void cleanup_all() {
        throw std::runtime_error("Not supported");
    }

    struct texture_unit_pool_t {
        bool freelist[32] = {1};
        uint8_t max_idx = 0;
        std::mutex freelist_mutex;
        std::thread watchdog;
        std::atomic_bool watchdog_stop = false;

        ~texture_unit_pool_t() {
            watchdog_stop = true;
            if (watchdog.joinable())
                watchdog.join();
        }
    } texture_unit_pool;

    bool tx_alloc(int &dst) {
        std::lock_guard<std::mutex> _(texture_unit_pool.freelist_mutex);
        for (uint8_t i = 0; i < texture_unit_pool.max_idx; ++i) {
            if (texture_unit_pool.freelist[i]) {
                texture_unit_pool.freelist[i] = false;
                dst = i;
                return true;
            }
        }

        return false;
    }

    void tx_free(int &dst) {
        std::lock_guard<std::mutex> _(texture_unit_pool.freelist_mutex);
        texture_unit_pool.freelist[dst] = true;
    }

    void tx_allocator_init(int max) {
        std::fill(texture_unit_pool.freelist, texture_unit_pool.freelist + max, true);
        texture_unit_pool.max_idx = max;
    }

    void tx_bind(int tx) {
        glActiveTexture(GL_TEXTURE0 + tx);
    }
}
