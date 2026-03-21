#include <chrono>
#include "rocket/macros.hpp"
#if defined(ROCKETGE__Platform_Android)
    #include <GLES3/gl32.h>
    #include <EGL/egl.h>
#else
    #include <GL/glew.h>
#endif
#include <glm/ext/vector_float2.hpp>
#include <iostream>

#include "../include/rgl.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits>
#include <mutex>
#include <queue>
#include <rocket/modularity/window_backend.hpp>
#include <shader_provider.hpp>
#include <string>
#include "rocket/rgl.hpp"
#include "rocket/types.hpp"
#include "../../include/rocket/runtime.hpp"
#include "rocket/window.hpp"
#include "util.hpp"
#include "glfnldr.hpp"
#ifdef ROCKETGE__Platform_Linux
#include <cpuid.h>
#else
#ifdef ROCKETGE__Platform_Android
#include <sys/system_properties.h>
#endif
#endif
#include "internal_types.hpp"
#include "intl_macros.hpp"

namespace glutil {
    std::string glenum_str(GLenum e) {
        switch (e) {
            default: {
                rocket::log("case not handled", "glutil", "glenum_str", "fatal");
                return "GLENUM_STR__CASE_NOT_HANDLED";
                r_assert(false);
            }
            // Blend Stuff
            case GL_ZERO: return "GL_ZERO";
            case GL_SRC_COLOR: return "GL_SRC_COLOR";
            case GL_ONE_MINUS_SRC_COLOR: return "GL_ONE_MINUS_SRC_COLOR";
            case GL_DST_COLOR: return "GL_DST_COLOR";
            case GL_ONE_MINUS_DST_COLOR: return "GL_ONE_MINUS_DST_COLOR";
            case GL_SRC_ALPHA: return "GL_SRC_ALPHA";
            case GL_ONE_MINUS_SRC_ALPHA: return "GL_ONE_MINUS_SRC_ALPHA";
            case GL_DST_ALPHA: return "GL_DST_ALPHA";
            case GL_ONE_MINUS_DST_ALPHA: return "GL_ONE_MINUS_DST_ALPHA";
            case GL_SRC_ALPHA_SATURATE: return "GL_SRC_ALPHA_SATURATE";
            case GL_CONSTANT_COLOR: return "GL_CONSTANT_COLOR";
            case GL_ONE_MINUS_CONSTANT_COLOR: return "GL_ONE_MINUS_CONSTANT_COLOR";
            case GL_CONSTANT_ALPHA: return "GL_CONSTANT_ALPHA";
            case GL_ONE_MINUS_CONSTANT_ALPHA: return "GL_ONE_MINUS_CONSTANT_ALPHA";
#ifdef ROCKETGE__Platform_Desktop
            case GL_SRC1_COLOR: return "GL_SRC1_COLOR";
            case GL_ONE_MINUS_SRC1_COLOR: return "GL_ONE_MINUS_SRC1_COLOR";
#endif
        }
    }
}

namespace rgl {
    scoped_gl_texture_t::scoped_gl_texture_t() {
        glGenTextures(1, &id);
    }

    int scoped_gl_texture_t::bind() {
        this->had_allocd_unit_handle = true;
        alloc_texture_unit(this->unit_handle);
        glActiveTexture(this->unit_handle.unit);
        glBindTexture(GL_TEXTURE_2D, this->id);

        return this->unit_handle.unit;
    }

    scoped_gl_texture_t::~scoped_gl_texture_t() {
        glDeleteTextures(1, &this->id);
        if (this->had_allocd_unit_handle)
            free_texture_unit(this->unit_handle);
    }
}

namespace rgl {
    std::pair<vao_t, vbo_t> rectVO = {0, 0};
    std::pair<vao_t, vbo_t> textureVO = {0, 0};
    std::pair<vao_t, vbo_t> textVO = {0, 0};

    fbo_t active_fbo = rGL_FBO_INVALID;
    rocket::vec2f_t viewport_size;
    rocket::vec2f_t viewport_offset = { 0, 0 };
    int max_tx_size = 0;

    rocket::native_window_t *gl_main_ctx;

    rocket::native_window_t *get_main_context() {
        return gl_main_ctx;
    }

    struct {
        bool freelist[32] = {1};
        uint8_t max_idx = 0;
        std::mutex freelist_mutex;
    } texture_unit_pool;

    bool alloc_texture_unit(texture_unit_handle_t &dst) {
        std::lock_guard<std::mutex> _(texture_unit_pool.freelist_mutex);
        for (uint8_t i = 0; i < texture_unit_pool.max_idx; ++i) {
            if (texture_unit_pool.freelist[i]) {
                texture_unit_pool.freelist[i] = false;
                dst = texture_unit_handle_t { static_cast<unsigned int>(GL_TEXTURE0) + i };
                
                return true;
            }
        }

        return false;
    }

    void free_texture_unit(texture_unit_handle_t &dst) {
        std::lock_guard<std::mutex> _(texture_unit_pool.freelist_mutex);
        r_assert(texture_unit_pool.freelist[dst.unit - GL_TEXTURE0] == false);
        texture_unit_pool.freelist[dst.unit - GL_TEXTURE0] = true;
        dst.unit = std::numeric_limits<unsigned int>::max();
    }

    std::string float_str(float value) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6) << value; // control precision
        std::string s = oss.str();

        // strip trailing zeros and possibly the decimal point
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (s.back() == '.') s.pop_back();
        return s;
    }

    void init_vo_all() {
        // rect quad
        glGenVertexArrays(1, &rectVO.first);
        glGenBuffers(1, &rectVO.second);

        float square_vertices[] = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f
        };

        glBindVertexArray(rectVO.first);
        glBindBuffer(GL_ARRAY_BUFFER, rectVO.second);
        glBufferData(GL_ARRAY_BUFFER, sizeof(square_vertices), square_vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // textured rect quad
        glGenVertexArrays(1, &textureVO.first);
        glGenBuffers(1, &textureVO.second);

        glBindVertexArray(textureVO.first);
        glBindBuffer(GL_ARRAY_BUFFER, textureVO.second);
        glBufferData(GL_ARRAY_BUFFER, sizeof(square_vertices), square_vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // text quads (dynamic VBO)
        glGenVertexArrays(1, &textVO.first);
        glGenBuffers(1, &textVO.second);

        glBindVertexArray(textVO.first);
        glBindBuffer(GL_ARRAY_BUFFER, textVO.second);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0); // aPos
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        glEnableVertexAttribArray(1); // aTex
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    std::pair<vao_t, vbo_t> compile_vo(
        const std::array<float, 12>& square_vertices,
        GLenum draw_type,
        int stride_size
    ) {
        vao_t vao;
        vbo_t vbo;

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glBufferData(GL_ARRAY_BUFFER, sizeof(square_vertices), square_vertices.data(), draw_type);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride_size * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        return { vao, vbo };
    }

    std::pair<vao_t, vbo_t> compile_vo(
        const std::vector<float> &vertices,
        GLenum draw_type,
        int stride_size
    ) {
        vao_t vao;
        vbo_t vbo;

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), draw_type);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride_size * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        return { vao, vbo };
    }

    static std::unordered_map<std::string, std::pair<vao_t, vbo_t>> cached_square_vertices_vos;

    std::pair<vao_t, vbo_t> cache_compile_vo(
        std::string use,
        const std::array<float, 12>& square_vertices,
        GLenum draw_type,
        int stride_size
    ) {
        std::string key = use;

        auto it = cached_square_vertices_vos.find(key);
        if (it != cached_square_vertices_vos.end()) {
            return it->second;
        } else {
            rocket::log("VO cache miss, compiling...", "rgl", "cache_compile_vo", "debug");
            auto vo = rgl::compile_vo(square_vertices, draw_type, stride_size);
            cached_square_vertices_vos[key] = vo;
            return vo;
        }
    }

    static std::unordered_map<std::string, std::pair<vao_t, vbo_t>> cached_float_vertices_vos;

    std::pair<vao_t, vbo_t> cache_compile_vo(
        std::string use,
        const std::vector<float> &vertices,
        GLenum draw_type,
        int stride_size
    ) {
        std::string key = use;

        auto it = cached_float_vertices_vos.find(key);
        if (it != cached_float_vertices_vos.end()) {
            return it->second;
        } else {
            rocket::log("VO cache miss, compiling...", "rgl", "cache_compile_vo", "debug");
            auto vo = rgl::compile_vo(vertices, draw_type, stride_size);
            cached_float_vertices_vos[key] = vo;
            return vo;
        }
    }

    std::pair<vao_t, vbo_t> get_text_vos() {
        return textVO;
    }

    std::pair<vao_t, vbo_t> get_quad_vos() {
        return rectVO;
    }

    std::pair<vao_t, vbo_t> get_txquad_vos() {
        return textureVO;
    }

    std::string bool_to_str(bool b, bool caps = true) { 
        return b ? "[TRUE]" : "[FALSE]";
    }

    std::vector<std::string> dump_gl_state() {
        glstate_t state = rgl::save_state();

        return {
            "OpenGL State:",
            "   Blend: " + bool_to_str(state.blend_mode.enabled) + " ...",
            "     Src RGB: " + glutil::glenum_str(state.blend_mode.src_rgb),
            "     Dest RGB: " + glutil::glenum_str(state.blend_mode.dst_rgb),
            "     Src Alpha: " + glutil::glenum_str(state.blend_mode.src_alpha),
            "     Dest Alpha: " + glutil::glenum_str(state.blend_mode.dst_alpha),
            "   Bound Framebuffer:",
            "     fboID: " + std::to_string(state.bound_framebuffer.fbo),
            "     fboColorTex: " + std::to_string(state.bound_framebuffer.color_tex),
            "   Bound VertexObject:",
            "     vaoID: " + std::to_string(state.bound_vo.first),
            "     vboID: " + std::to_string(state.bound_vo.second),
            "   Active Shader ID: " + std::to_string(state.active_shader),
        };
    }

    void init_gl_wtd() {
        glfnldr::init(glfnldr::backend_t::glew);
        glViewport(0, 0, viewport_size.x, viewport_size.y);
        init_vo_all();
    }

    
    std::string get_cpu_name() {
#ifdef ROCKETGE__Platform_Linux
        char cpu[64] = {};
        unsigned int info[4];
        __get_cpuid(0x80000002, &info[0], &info[1], &info[2], &info[3]); memcpy(cpu, info, 16);
        __get_cpuid(0x80000003, &info[0], &info[1], &info[2], &info[3]); memcpy(cpu+16, info, 16);
        __get_cpuid(0x80000004, &info[0], &info[1], &info[2], &info[3]); memcpy(cpu+32, info, 16);
        return std::string(cpu);
#elifdef ROCKETGE__Platform_Android
        char value[PROP_VALUE_MAX];
        __system_property_get("ro.soc.model", value);
        return std::string(value);
#endif
        return std::string("Querying CPU Name is not supported on this platform");
    }

    static std::unordered_map<rgl::shader_use_t, rgl::shader_program_t> shader_cache;
    std::vector<std::string> init_gl(rocket::vec2f_t viewport_size, glfnldr::backend_t backend, rocket::window_backend_i *win) {
        ::rgl::viewport_size = viewport_size;
        std::unordered_map<rgl::shader_use_t, rgl::shader_program_t>().swap(shader_cache);

        auto cli_args = util::get_clistate();

        if (!glfnldr::init(backend)) {
            rocket::log("OpenGL functions could not be loaded", "rgl", "init_gl", "fatal");
            rocket::exit(1);
            return {
                "?OpenGL functions could not be loaded"
            };
        }

#ifdef ROCKETGE__Platform_Desktop
        if (glGenBuffers == nullptr) {
            rocket::log("OpenGL functions could not be loaded (RUNTIME FAILURE)", "rgl", "init_gl", "fatal");
            rocket::exit(1);
            return {
                "?OpenGL functions could not be loaded (RUNTIME FAILURE)"
            };
        }
#endif

        glViewport(0, 0, viewport_size.x, viewport_size.y);
        init_vo_all();

        while (glGetError() != GL_NO_ERROR) {};

        glEnable(GL_BLEND);

        bool gl_multisample = false;
        int gl_samples = 0;
        if (win->flags.msaa_samples > 0) {
#ifdef ROCKETGE__Platform_Desktop
            glEnable(GL_MULTISAMPLE);
#endif
            gl_multisample = true;
            glGetIntegerv(GL_SAMPLES, &gl_samples);
        }

        GLenum sfactor = GL_SRC_ALPHA;
        GLenum dfactor = GL_ONE_MINUS_SRC_ALPHA;
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        std::string gl_blendfunc = glutil::glenum_str(sfactor) + ", " + glutil::glenum_str(dfactor);

        // Enable SRGB framebuffer if supported
#ifdef ROCKETGE__Platform_Desktop
        glEnable(GL_FRAMEBUFFER_SRGB);
#endif
        std::string gl_framebuffer_mode = "sRGB";

        // Query max texture size
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tx_size);

        int flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
#ifdef ROCKETGE__Platform_Desktop
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
            glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
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

                std::vector<std::string> log_messages2 = dump_gl_state();

                rocket::get_opengl_error_callback()(typeStr, sevStr, id, message, srcStr);
                for (auto &l : log_messages) {
                    rocket::log(l, "OpenGL", "ContextVerifier", "error");
                }
                for (auto &l : log_messages2) {
                    rocket::log(l, "OpenGL", "ContextVerifier", "error");
                }
            }, nullptr);
        }
#ifdef RocketRuntime_VerifyShaderLoading
        shader_program_t prg = get_paramaterized_quad({0.f, 0.f}, {1.f, 1.f}, rocket::rgba_color::red(), 0.f, 0.f);
        draw_shader(prg, shader_use_t::rect);

        rgl::texture_id_t glid;
        glGenTextures(1, &glid);
        glBindTexture(GL_TEXTURE_2D, glid);
        uint8_t pixel[4] = { 0, 0, 0, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glActiveTexture(GL_TEXTURE0);
        prg = get_paramaterized_textured_quad({0.f, 0.f}, {1.f, 1.f}, 0.f, 0.f);
        glUniform1i(glGetUniformLocation(prg, "u_texture"), 0);
        draw_shader(prg, shader_use_t::textured_rect);

        glDeleteTextures(1, &glid);
        glBindTexture(GL_TEXTURE_2D, 0);
#endif
#endif

        auto gl_get_integer = [](GLenum e) -> int {
            int val;
            glGetIntegerv(e, &val);
            return val;
        };

        int major = gl_get_integer(GL_MAJOR_VERSION);
        int minor = gl_get_integer(GL_MINOR_VERSION);

        std::string gl_version_string = std::to_string(major) + "." + std::to_string(minor);

        std::string gl_driver_string = std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));

        int max_available_tx_units = gl_get_integer(GL_MAX_TEXTURE_IMAGE_UNITS);

        bool gpu_is_modern = true;
        if (max_available_tx_units < 32 || max_tx_size < 8192) {
            gpu_is_modern = false;
        }

        int loaded_extensions = gl_get_integer(GL_NUM_EXTENSIONS);

        std::string gpu_name = std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
        std::string gpu_vendor = std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));

        gl_main_ctx = win->handle;

        // Collect init logs
        std::vector<std::string> logs = {
            "OpenGL Info:",
            "Driver: " + gl_driver_string,
            "Version: " + gl_version_string,
            "GLSL Version: " + std::string((const char *) glGetString(GL_SHADING_LANGUAGE_VERSION)),
            "" + std::to_string(loaded_extensions) + " extensions loaded",
            "Multisampling: " + bool_to_str(gl_multisample) + (gl_samples > 0 ? (" (" + std::to_string(gl_samples) + "x)") : ""),
            "Context Verifier: " + bool_to_str(flags & GL_CONTEXT_FLAG_DEBUG_BIT),
            "GPU:",
            "  Name: " + gpu_name,
            "  Vendor: "  + gpu_vendor,
            "CPU: " + get_cpu_name() + " " + "(" + std::to_string(std::thread::hardware_concurrency()) + ")"
        };

        // !LogMessage: Warning
        // ?LogMessage: Error

        texture_unit_pool.max_idx = max_available_tx_units;
        std::fill(std::begin(texture_unit_pool.freelist), std::begin(texture_unit_pool.freelist) + max_available_tx_units, true);

        if (!gpu_is_modern) {
            logs.push_back("!GPU does not fully support required features. Expect reduced performance or glitches");
            logs.push_back("!MAX_TX_UNITS: " + std::to_string(max_available_tx_units));
            logs.push_back("!MAX_TX_SIZE: " + std::to_string(max_tx_size));
        }

        if (gpu_name.contains("llvmpipe") || gpu_name.contains("soft")) {
            logs.push_back("!using software OpenGL renderer");
        }

        return logs;
    }

    std::queue<std::function<void()>> scheduled;
    std::mutex scheduled_mutex;

    void schedule_gl(std::function<void()> fn) {
        std::lock_guard<std::mutex> _(scheduled_mutex);
        scheduled.push(fn);
    }

    void cleanup_all() {
        std::lock_guard<std::mutex> _(scheduled_mutex);
        if (scheduled.size() > 0) {
            rocket::log("Exiting with pending scheduled operations", "rgl", "cleanup_all", "warn");
        }
    }

    bool is_active_any_fbo() {
        GLint fbo = rGL_FBO_INVALID.fbo;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);

        return fbo != rGL_FBO_INVALID.fbo;
    }

    fbo_t create_fbo() {
        fbo_t fbo;
        glGenFramebuffers(1, &fbo.fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);

        glGenTextures(1, &fbo.color_tex);
        glBindTexture(GL_TEXTURE_2D, fbo.color_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, viewport_size.x, viewport_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo.color_tex, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            rocket::log("Failed to create custom framebuffer", "OpenGL", "Framebuffer", "error");
            return rGL_FBO_INVALID;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        rocket::log("FBO Created with ID: " + std::to_string(fbo.fbo), "rgl", "create_fbo", "debug");
        return fbo;
    }

    void use_fbo(fbo_t f) {
        glBindFramebuffer(GL_FRAMEBUFFER, f.fbo);

        active_fbo = f;
    }

    void delete_fbo(fbo_t f) {
        glDeleteFramebuffers(1, &f.fbo);
        glDeleteTextures(1, &f.color_tex);
        rocket::log("FBO Deleted with ID: " + std::to_string(f.fbo), "rgl", "delete_fbo", "debug");
    }

    void reset_to_default_fbo() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        active_fbo = rGL_FBO_INVALID;
    }

    fbo_t get_active_fbo() {
        return active_fbo;
    }

    rgl::shader_program_t load_shader_generic(const char *vsrc, const char *fsrc) {
        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vsrc, nullptr);
        glCompileShader(vs);

        GLint success;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLint logLen;
            glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &logLen);
            std::string log(logLen, '\0');
            glGetShaderInfoLog(vs, logLen, nullptr, log.data());
            rocket::log("Vertex shader compile failed: " + log, "OpenGL", "ShaderCompiler", "error");
        }

        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fsrc, nullptr);
        glCompileShader(fs);

        GLint s2;
        glGetShaderiv(fs, GL_COMPILE_STATUS, &s2);
        if (!s2) {
            GLint logLen;
            glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &logLen);
            std::string log(logLen, '\0');
            glGetShaderInfoLog(fs, logLen, nullptr, log.data());
            rocket::log("Fragment shader compile failed: " + log, "OpenGL", "ShaderCompiler", "error");
        }

        rgl::shader_program_t pg = glCreateProgram();
        glAttachShader(pg, vs);
        glAttachShader(pg, fs);
        glLinkProgram(pg);

        glGetProgramiv(pg, GL_LINK_STATUS, &success);
        if (!success) {
            GLint logLen;
            glGetProgramiv(pg, GL_INFO_LOG_LENGTH, &logLen);
            std::string log(logLen, '\0');
            glGetProgramInfoLog(pg, logLen, nullptr, log.data());
            rocket::log("Shader program link failed: " + log, "OpenGL", "ShaderCompiler", "error");
        } 

        glDeleteShader(vs);
        glDeleteShader(fs);
        return pg;
    }

    std::unordered_map<std::string, rgl::shader_program_t> cachecmp_shader_cache;

    rgl::shader_program_t cache_compile_shader(const char *vsrc, const char *fsrc) {
        std::string key = std::string(vsrc) + "#" + fsrc;

        auto it = cachecmp_shader_cache.find(key);
        if (it != cachecmp_shader_cache.end()) {
            return it->second;
        } else {
            rocket::log("Shader cache miss, compiling...", "rgl", "cache_compile_shader", "debug");
            rgl::shader_program_t pg = load_shader_generic(vsrc, fsrc);
            cachecmp_shader_cache[key] = pg;
            return pg;
        }
    }

    rgl::shader_program_t nocache_compile_shader(const char *vsrc, const char *fsrc) {
        return load_shader_generic(vsrc, fsrc);
    }

    void log_default_shader_compiled(std::string shader_name) {
        rocket::log("Shader '" + shader_name + "' compiled successfully", "rgl", "lazyshader", "info");
    }

    rgl::shader_program_t init_shader(rgl::shader_use_t use) {
        switch (use) {
            case rgl::shader_use_t::rect:
                shader_cache[use] = rocket::get_shader(rocket::shader_id_t::rectangle);
                break;
            case rgl::shader_use_t::text:
                shader_cache[use] = rocket::get_shader(rocket::shader_id_t::text);
                break;
            case rgl::shader_use_t::textured_rect:
                shader_cache[use] = rocket::get_shader(rocket::shader_id_t::textured_rectangle);
                break;
            default:
                rocket::log("unknown shader use", "rgl", "init_shader", "error");
                break;
        }

        return shader_cache[use];
    }

    rgl::shader_program_t get_shader(shader_use_t use) {
        return init_shader(use);
    }

    shader_program_t get_paramaterized_quad(
        const rocket::vec2f_t &pos,
        const rocket::vec2f_t &size,
        const rocket::rgba_color &color,
        float rotation,
        float roundedness
    ) {
        rgl::shader_program_t pg = init_shader(rgl::shader_use_t::rect);

        glm::mat4 projection = glm::ortho(0.f, viewport_size.x, viewport_size.y, 0.f, -1.f, 1.f);

        float cx = pos.x + size.x * 0.5f;
        float cy = pos.y + size.y * 0.5f;

        glm::mat4 transform = projection
            * glm::translate(glm::mat4(1.0f), glm::vec3(cx, cy, 0.0f))
            * glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f))
            * glm::translate(glm::mat4(1.0f), glm::vec3(-size.x * 0.5f, -size.y * 0.5f, 0.0f))
            * glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));

        auto nm = color.normalize();

        glUseProgram(pg);
        glUniformMatrix4fv(glGetUniformLocation(pg, "u_transform"), 1, GL_FALSE, glm::value_ptr(transform));
        glUniform4f(glGetUniformLocation(pg, "u_color"), nm.x, nm.y, nm.z, nm.w);
        glUniform2f(glGetUniformLocation(pg, "u_size"), size.x, size.y);
        glUniform1f(glGetUniformLocation(pg, "u_radius"), roundedness);

        return pg;
    }

    shader_program_t get_paramaterized_textured_quad(
        const rocket::vec2f_t &pos,
        const rocket::vec2f_t &size,
        float rotation,
        float roundedness
    ) {
        rgl::shader_program_t pg = init_shader(rgl::shader_use_t::textured_rect);

        glm::mat4 projection = glm::ortho(0.f, viewport_size.x, viewport_size.y, 0.f, -1.f, 1.f);

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

        return pg;
    }

    std::pair<glm::vec2, glm::vec2> get_rect_uv_bounds(const glm::vec2& pos, const glm::vec2& size, const glm::mat4& mvp) {
        // Local corners in world-space
        std::array<glm::vec2, 4> corners = {
            pos,
            pos + glm::vec2(size.x, 0.0f),
            pos + glm::vec2(0.0f, size.y),
            pos + size
        };

        std::array<glm::vec2, 4> uvs;

        // Transform each corner
        for (int i = 0; i < 4; i++) {
            glm::vec4 clip = mvp * glm::vec4(corners[i], 0.0f, 1.0f);

            // Ortho → clip.w == 1, but safe to divide
            glm::vec3 ndc = glm::vec3(clip) / clip.w;

            // Convert NDC (-1..1) → UV (0..1)
            uvs[i].x = ndc.x * 0.5f + 0.5f;
            uvs[i].y = ndc.y * 0.5f + 0.5f;
        }

        // Find bounds
        float u_min = std::min({uvs[0].x, uvs[1].x, uvs[2].x, uvs[3].x});
        float u_max = std::max({uvs[0].x, uvs[1].x, uvs[2].x, uvs[3].x});
        float v_min = std::min({uvs[0].y, uvs[1].y, uvs[2].y, uvs[3].y});
        float v_max = std::max({uvs[0].y, uvs[1].y, uvs[2].y, uvs[3].y});

        return { glm::vec2(u_min, v_min), glm::vec2(u_max, v_max) };
    }

    void draw_shader(rgl::shader_program_t pg, rgl::shader_use_t use) {
        glUseProgram(pg);

        switch (use) {
            case rgl::shader_use_t::rect:
                glBindVertexArray(rectVO.first);
                break;
            case rgl::shader_use_t::text:
                glBindVertexArray(textVO.first);
                break;
            case rgl::shader_use_t::textured_rect:
                glBindVertexArray(textureVO.first);
                break;
            default:
                rocket::log("unknown shader use", "rgl", "draw_shader", "error");
                break;
        }

        gl_draw_arrays(GL_TRIANGLES, 0, 6);
    }

    void draw_shader(rgl::shader_program_t pg, vao_t vao, vbo_t vbo) {
        glUseProgram(pg);

        glBindVertexArray(vao);
        gl_draw_arrays(GL_TRIANGLES, 0, 6);
    }

    void update_viewport(const rocket::vec2f_t &size) {
        viewport_size = size;
        glViewport(0, 0, size.x, size.y);
    }

    void update_viewport(const rocket::vec2f_t &offset, const rocket::vec2f_t &size) {
        if (viewport_size != size) {
            rocket::log("Viewport size changed: [" + std::to_string((int) viewport_size.x) + "x" + std::to_string((int)viewport_size.y) + "] to [" + std::to_string((int) size.x) + "x" + std::to_string((int) size.y) + "]", "rgl", "update_viewport", "trace");
        }
        viewport_size   = size;
        viewport_offset = offset;

        int flipped_y = size.y - (offset.y + size.y);

        glViewport(offset.x, flipped_y, size.x, size.y);
    }

    void gl_viewport(const rocket::vec2f_t &offset, const rocket::vec2f_t &size) {
        glViewport(offset.x, offset.y, size.x, size.y);
    }

    void gl_clear(unsigned int gl_flags, rocket::rgba_color clear_color) {
        auto col = clear_color.normalize();
        glClearColor(col.x, col.y, col.z, col.w);
        glClear(gl_flags);
    }

    rocket::vec2f_t get_viewport_size() {
        return viewport_size;
    }

    shader_location_t get_shader_location(shader_program_t sp, const char *name) {
        return glGetUniformLocation(sp, name);
    }

    shader_location_t get_shader_location(shader_program_t sp, const std::string &name) {
        return get_shader_location(sp, name.c_str());
    }

    void gl_draw_arrays(GLenum mode, GLint first, GLsizei count) {
        add_frame_metrics_data_drawcalls(1);
        add_frame_metrics_data_tricount(count / 3);
        glDrawArrays(mode, first, count);
    }

    void run_all_scheduled_gl() {
        std::queue<std::function<void()>> local;
        {
            std::lock_guard<std::mutex> _(scheduled_mutex);
            std::swap(local, scheduled);
        }

        while (!local.empty()) {
            auto fn = local.front();
            fn();
            local.pop();
        }
    }

    rgl::shader_program_t get_fxaa_simplified_shader() {
        return rocket::get_shader(rocket::shader_id_t::fxaa);
    }

    rgl::glstate_t save_state() {
        rgl::glstate_t state;
        state.bound_framebuffer = rgl::get_active_fbo();
        GLint active_txunit;
        glGetIntegerv(GL_ACTIVE_TEXTURE, &active_txunit);
        state.bound_texture_unit = active_txunit - GL_TEXTURE0;

        rgl::vao_t vao;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, std::bit_cast<GLint*>(&vao));
        rgl::vbo_t vbo;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, std::bit_cast<GLint*>(&vbo));
        state.bound_vo = std::make_pair(vao, vbo);

        glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&state.active_shader));

        glGetIntegerv(GL_BLEND_SRC_RGB, reinterpret_cast<GLint*>(&state.blend_mode.src_rgb));
        glGetIntegerv(GL_BLEND_DST_RGB, reinterpret_cast<GLint*>(&state.blend_mode.dst_rgb));
        glGetIntegerv(GL_BLEND_SRC_ALPHA, reinterpret_cast<GLint*>(&state.blend_mode.src_alpha));
        glGetIntegerv(GL_BLEND_DST_ALPHA, reinterpret_cast<GLint*>(&state.blend_mode.dst_alpha));

        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&state.bound_fbo.fbo));
        glGetFramebufferAttachmentParameteriv(
            GL_DRAW_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
            reinterpret_cast<GLint*>(&state.bound_fbo.color_tex)
        );

        GLboolean blend_enabled = glIsEnabled(GL_BLEND);
        state.blend_mode.enabled = blend_enabled;
        return state;
    }

    void restore_state(rgl::glstate_t state) {
        rgl::use_fbo(state.bound_framebuffer);
        glActiveTexture(GL_TEXTURE0 + state.bound_texture_unit);
        if (state.bound_vo.first != rGL_VAO_INVALID && state.bound_vo.second != rGL_VBO_INVALID) {
            glBindVertexArray(state.bound_vo.first);
            glBindBuffer(GL_ARRAY_BUFFER, state.bound_vo.second);
        }

        if (state.active_shader != rGL_SHADER_INVALID) {
            glUseProgram(state.active_shader);
        }

        if (state.blend_mode.enabled) {
            glEnable(GL_BLEND);
            glBlendFuncSeparate(state.blend_mode.src_rgb, state.blend_mode.dst_rgb, state.blend_mode.src_alpha, state.blend_mode.dst_alpha);
        }

        if (state.bound_fbo.fbo != 0)
            glBindFramebuffer(GL_FRAMEBUFFER, state.bound_fbo.fbo);
    }
    void compile_all_default_shaders() {
        rocket::shader_provider_compile_all();
    }

    draw_metrics_t metrics;

    void update_draw_metrics_data(float frametime, float fps) {
        static float max_frametime_so_far = 0;
        static float min_frametime_so_far = 0;
        static float min_fps_so_far = 0;
        static float max_fps_so_far = 0;

        max_frametime_so_far = std::max(max_frametime_so_far, frametime);
        min_frametime_so_far = std::min(min_frametime_so_far, frametime);
        min_fps_so_far = std::min(min_fps_so_far, fps);
        max_fps_so_far = std::max(max_fps_so_far, fps);

        metrics.max_fps = max_fps_so_far;
        metrics.max_frametime = max_frametime_so_far;

        metrics.min_fps = min_fps_so_far;
        metrics.min_frametime = min_frametime_so_far;


        static float alpha = 0.1; // 0.1 = smooth, 0.5 = more reactive

        metrics.avg_fps = metrics.avg_fps + alpha * (fps - metrics.avg_fps);
        metrics.avg_frametime = metrics.avg_frametime + alpha * (frametime - metrics.avg_frametime);
    }

    frame_metrics_t fmetrics;

    void update_frame_metrics_data(frame_metrics_t metrics) {
        fmetrics = metrics;
    }
    void add_frame_metrics_data_drawcalls(int n) {
        fmetrics.drawcalls += n;
    }
    void add_frame_metrics_data_tricount(int n) {
        fmetrics.tricount += n;
    }
    void add_frame_metrics_data_skipped_drawcalls(int n) {
        fmetrics.skipped_drawcalls += n;
    }
    frame_metrics_t get_frame_metrics() {
        return fmetrics;
    }
    void reset_frame_metrics() {
        fmetrics = {};
    }

    draw_metrics_t get_draw_metrics() {
        return metrics;
    }

    void reset() {
        gl_main_ctx = nullptr;
        fmetrics = {};
        metrics = {};

        rectVO = {};
        textureVO = {};
        textVO = {};
        active_fbo = rGL_FBO_INVALID;
        viewport_size = {};
        viewport_offset = {};
        max_tx_size = 0;
        
        shader_cache.clear();
        cachecmp_shader_cache.clear();
        cached_square_vertices_vos.clear();
        cached_float_vertices_vos.clear();

        {
            std::lock_guard<std::mutex> _(texture_unit_pool.freelist_mutex);
            texture_unit_pool.max_idx = 0;
            std::fill(std::begin(texture_unit_pool.freelist), std::end(texture_unit_pool.freelist), false);
        }

        {
            std::lock_guard<std::mutex> _(scheduled_mutex);
            while (scheduled.size() != 0) scheduled.pop();
        }
    }
    
    // --- Float uniforms ---
    void gl_uniform1f(shader_program_t prog, GLint location, float v0) {
        glUseProgram(prog);
        glUniform1f(location, v0);
    }

    void gl_uniform2f(shader_program_t prog, GLint location, float v0, float v1) {
        glUseProgram(prog);
        glUniform2f(location, v0, v1);
    }

    void gl_uniform3f(shader_program_t prog, GLint location, float v0, float v1, float v2) {
        glUseProgram(prog);
        glUniform3f(location, v0, v1, v2);
    }

    void gl_uniform4f(shader_program_t prog, GLint location, float v0, float v1, float v2, float v3) {
        glUseProgram(prog);
        glUniform4f(location, v0, v1, v2, v3);
    }

    // --- Int uniforms ---
    void gl_uniform1i(shader_program_t prog, GLint location, int v0) {
        glUseProgram(prog);
        glUniform1i(location, v0);
    }

    void gl_uniform2i(shader_program_t prog, GLint location, int v0, int v1) {
        glUseProgram(prog);
        glUniform2i(location, v0, v1);
    }

    void gl_uniform3i(shader_program_t prog, GLint location, int v0, int v1, int v2) {
        glUseProgram(prog);
        glUniform3i(location, v0, v1, v2);
    }

    void gl_uniform4i(shader_program_t prog, GLint location, int v0, int v1, int v2, int v3) {
        glUseProgram(prog);
        glUniform4i(location, v0, v1, v2, v3);
    }

    void bind_texture_unit(rgl::texture_unit_t unit) {
        glActiveTexture(unit);
    }

    void bind_texture(rgl::texture_id_t tx) {
        glBindTexture(GL_TEXTURE_2D, tx);
    }
}
