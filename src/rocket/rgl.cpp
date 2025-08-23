#include <iostream>

#include "../include/rgl.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "rocket/asset.hpp"
#include "rocket/types.hpp"
#include "../../include/rocket/runtime.hpp"
#include "util.hpp"

#ifdef RocketRuntime_DEBUG
    #define GL_CHECK(x) \
        x; \
        { \
            GLenum err = glGetError(); \
            if (err != GL_NO_ERROR) { \
                std::cerr << "[GL ERROR] " << #x << " -> " << util::format_error("OpenGL error", err, "OpenGL", "warn") << std::endl; \
            } \
        }
#else
    #define GL_CHECK(x) x
#endif

namespace rgl {
    std::pair<vao_t, vbo_t> rectVO = {0, 0};
    std::pair<vao_t, vbo_t> textureVO = {0, 0};
    std::pair<vao_t, vbo_t> textVO = {0, 0};

    rocket::vec2f_t viewport_size;
    int max_tx_size = 0;

    void init_vo_all() {
        // rect quad
        GL_CHECK(glGenVertexArrays(1, &rectVO.first));
        GL_CHECK(glGenBuffers(1, &rectVO.second));

        float square_vertices[] = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f
        };

        GL_CHECK(glBindVertexArray(rectVO.first));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, rectVO.second));
        GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(square_vertices), square_vertices, GL_STATIC_DRAW));
        GL_CHECK(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0));
        GL_CHECK(glEnableVertexAttribArray(0));
        GL_CHECK(glBindVertexArray(0));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

        // textured rect quad
        GL_CHECK(glGenVertexArrays(1, &textureVO.first));
        GL_CHECK(glGenBuffers(1, &textureVO.second));

        GL_CHECK(glBindVertexArray(textureVO.first));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, textureVO.second));
        GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(square_vertices), square_vertices, GL_STATIC_DRAW));
        GL_CHECK(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0));
        GL_CHECK(glEnableVertexAttribArray(0));
        GL_CHECK(glBindVertexArray(0));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

        // text quads (dynamic VBO)
        GL_CHECK(glGenVertexArrays(1, &textVO.first));
        GL_CHECK(glGenBuffers(1, &textVO.second));

        GL_CHECK(glBindVertexArray(textVO.first));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, textVO.second));
        GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW));
        GL_CHECK(glEnableVertexAttribArray(0)); // aPos
        GL_CHECK(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0));
        GL_CHECK(glEnableVertexAttribArray(1)); // aTex
        GL_CHECK(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float))));
        GL_CHECK(glBindVertexArray(0));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
    }

    std::pair<vao_t, vbo_t> get_text_vos() {
        return textVO;
    }

    std::vector<std::string> init_gl(const rocket::vec2f_t viewport_size) {
        ::rgl::viewport_size = viewport_size;

        // Init GLEW
        glewExperimental = true;
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            const GLubyte* err_str = glewGetErrorString(err);
            rocket::log_error(reinterpret_cast<const char*>(err_str), err, "glew", "warning");
        }

        // Setup viewport
        glViewport(0, 0, viewport_size.x, viewport_size.y);
        init_vo_all();

        // Clear GL errors
        while (glGetError() != GL_NO_ERROR) {};

        // GL state toggles
        glEnable(GL_BLEND);
        bool gl_blend = true;

        glEnable(GL_MULTISAMPLE);
        bool gl_multisample = true;

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        std::string gl_blendfunc = "GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA";

        // Enable SRGB framebuffer if supported
        glEnable(GL_FRAMEBUFFER_SRGB);
        bool gl_srgb = true;

        // Query max texture size
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tx_size);

#ifdef RocketRuntime_DEBUG
        int flags; 
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // so you see them immediately
            glDebugMessageCallback(
                [](GLenum source, GLenum type, GLuint id, GLenum severity,
                   GLsizei length, const GLchar* message, const void* userParam) {
                    fprintf(stderr, "[GL DEBUG] %s\n", message);
                },
                nullptr);
        }

        shader_program_t prg = get_paramaterized_quad({0.f, 0.f}, {1.f, 1.f}, rocket::rgba_color::red(), 0.f, 0.f);
        draw_shader(prg, shader_use_t::rect);
#endif

        float glversion = 0;
        int major, minor;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
        glversion = major + minor / 10.f;

        // Collect init logs
        std::vector<std::string> logs = {
            "GL Info:",
            "- GL Version: " + std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION))),
            "- GL Vendor: "  + std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR))),
            "- Using GL Version: " + std::to_string(major) + "." + std::to_string(minor),
            "- GLSL Version: " + std::string((const char *) glGetString(GL_SHADING_LANGUAGE_VERSION)),

            "- GL Activated Functions:",
            " - GL_BLEND: [" + (gl_blend ? std::string("TRUE") : std::string("FALSE")) + "]",
            " - GL_MULTISAMPLE: [" + (gl_multisample ? std::string("TRUE") : std::string("FALSE")) + "]",
            " - GL_BLEND_FUNC: [" + gl_blendfunc + "]",
            " - GL_FRAMEBUFFER_SRGB: [" + (gl_srgb ? std::string("TRUE") : std::string("FALSE")) + "]",

            "- GL VAO/VBO Pairs Created:",
            " - rectVAO/VBO: [" + (rectVO.first != 0 && rectVO.second != 0
                                    ? std::string("TRUE") : std::string("FALSE")) + "]",
            " - txVAO/VBO: [" + (textureVO.first != 0 && textureVO.second != 0
                                    ? std::string("TRUE") : std::string("FALSE")) + "]",
            " - textVAO/VBO: [" + (textVO.first != 0 && textVO.second != 0
                                    ? std::string("TRUE") : std::string("FALSE")) + "]",

            "- GL GPU-Specific Values:",
            " - GL_MAX_TEXTURE_SIZE: " + std::to_string(max_tx_size) + " x " + std::to_string(max_tx_size),

            "- Viewport Size: " + std::to_string(viewport_size.x) + " x " + std::to_string(viewport_size.y)
        };

        return logs;
    }


    rgl::shader_program_t load_shader_generic(const char *vsrc, const char *fsrc) {
        GLuint vs = GL_CHECK(glCreateShader(GL_VERTEX_SHADER));
        GL_CHECK(glShaderSource(vs, 1, &vsrc, nullptr));
        GL_CHECK(glCompileShader(vs));

        GLint success;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLint logLen;
            glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &logLen);
            std::string log(logLen, '\0');
            glGetShaderInfoLog(vs, logLen, nullptr, log.data());
            std::cerr << "[GL SHADER ERROR] Vertex shader compile failed:\n" << log << std::endl;
        }

        GLuint fs = GL_CHECK(glCreateShader(GL_FRAGMENT_SHADER));
        GL_CHECK(glShaderSource(fs, 1, &fsrc, nullptr));
        GL_CHECK(glCompileShader(fs));

        GLint s2;
        glGetShaderiv(fs, GL_COMPILE_STATUS, &s2);
        if (!s2) {
            GLint logLen;
            glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &logLen);
            std::string log(logLen, '\0');
            glGetShaderInfoLog(fs, logLen, nullptr, log.data());
            std::cerr << "[GL SHADER ERROR] Fragment shader compile failed:\n" << log << std::endl;
        }

        rgl::shader_program_t pg = GL_CHECK(glCreateProgram());
        GL_CHECK(glAttachShader(pg, vs));
        GL_CHECK(glAttachShader(pg, fs));
        GL_CHECK(glLinkProgram(pg));

        glGetProgramiv(pg, GL_LINK_STATUS, &success);
        if (!success) {
            GLint logLen;
            glGetProgramiv(pg, GL_INFO_LOG_LENGTH, &logLen);
            std::string log(logLen, '\0');
            glGetProgramInfoLog(pg, logLen, nullptr, log.data());
            std::cerr << "[GL SHADER ERROR] Program link failed:\n" << log << std::endl;
        }

        GL_CHECK(glDeleteShader(vs));
        GL_CHECK(glDeleteShader(fs));

        return pg;
    }

    rgl::shader_program_t load_shader_textured_rect() {
        const char* vert_src = R"(
            #version 330 core
            layout(location = 0) in vec2 aPos;
            out vec2 v_uv;
            uniform mat4 u_transform;

            void main() {
                v_uv = aPos; // 0→1 quad coords
                gl_Position = u_transform * vec4(aPos, 0.0, 1.0);
            }
        )";

        const char* frag_src = R"(
            #version 330 core
            in vec2 v_uv;
            out vec4 FragColor;

            uniform sampler2D u_texture;
            uniform float u_radius; // fraction 0..1 of min size
            uniform vec2 u_size;    // rect size in pixels

            void main() {
                // Convert UV to local pixel space
                vec2 local_px = v_uv * u_size;

                // Convert fraction to pixel radius
                float radius_px = u_radius * 0.5 * min(u_size.x, u_size.y);

                // Distance from nearest edge
                vec2 cornerDist = min(local_px, u_size - local_px);

            // Distance from corner arc
            float dist = length(cornerDist - vec2(radius_px));

            // Anti-alias edge
            float edge_thickness = 1.0;
            float alpha = 1.0;

            if (cornerDist.x < radius_px && cornerDist.y < radius_px) {
                alpha = 1.0 - smoothstep(radius_px - edge_thickness, radius_px, dist);
            }

            vec4 texColor = texture(u_texture, v_uv);
            FragColor = vec4(texColor.rgb, texColor.a * alpha);
        })";

        return load_shader_generic(vert_src, frag_src);
    }

    rgl::shader_program_t load_shader_rect() {
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

        return load_shader_generic(vert_src, frag_src);
    }

    rgl::shader_program_t load_shader_text() {
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
            uniform vec3 u_color;
            uniform sampler2D u_texture;
            void main() {
                float alpha = texture(u_texture, TexCoord).r;
                // gamma correct to linear
                alpha = pow(alpha, 2.2);
                // sharpen edges
                alpha = pow(alpha, 0.5);
                // back to sRGB
                alpha = pow(alpha, 1.0 / 2.2);
                vec3 rgb = u_color * alpha;
                FragColor = vec4(rgb, alpha);
            }
        )";

        return load_shader_generic(vert_src, frag_src);
    }

    rgl::shader_program_t init_shader(rgl::shader_use_t use) {
        static std::unordered_map<rgl::shader_use_t, rgl::shader_program_t> shader_cache;

        switch (use) {
            case rgl::shader_use_t::rect:
                shader_cache[use] = load_shader_rect();
                break;
            case rgl::shader_use_t::text:
                shader_cache[use] = load_shader_text();
                break;
            case rgl::shader_use_t::textured_rect:
                shader_cache[use] = load_shader_textured_rect();
                break;
            default:
                rocket::log_error("unknown shader use", -1, "rgl", "fatal-to-function");
                break;
        }

        return shader_cache[use];
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

        GL_CHECK(glUseProgram(pg));
        GL_CHECK(glUniformMatrix4fv(glGetUniformLocation(pg, "u_transform"), 1, GL_FALSE, glm::value_ptr(transform)));
        GL_CHECK(glUniform4f(glGetUniformLocation(pg, "u_color"), nm.x, nm.y, nm.z, nm.w));
        GL_CHECK(glUniform2f(glGetUniformLocation(pg, "u_size"), size.x, size.y));
        GL_CHECK(glUniform1f(glGetUniformLocation(pg, "u_radius"), roundedness));

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

        GL_CHECK(glUseProgram(pg));
        GL_CHECK(glUniformMatrix4fv(glGetUniformLocation(pg, "u_transform"), 1, GL_FALSE, glm::value_ptr(transform)));
        GL_CHECK(glUniform2f(glGetUniformLocation(pg, "u_size"), size.x, size.y));
        GL_CHECK(glUniform1f(glGetUniformLocation(pg, "u_radius"), roundedness));

        return pg;
    }

    void draw_shader(rgl::shader_program_t pg, rgl::shader_use_t use) {
        GL_CHECK(glUseProgram(pg));

        switch (use) {
            case rgl::shader_use_t::rect:
                GL_CHECK(glBindVertexArray(rectVO.first));
                break;
            case rgl::shader_use_t::text:
                GL_CHECK(glBindVertexArray(textVO.first));
                break;
            case rgl::shader_use_t::textured_rect:
                GL_CHECK(glBindVertexArray(textureVO.first));
                break;
            default:
                rocket::log_error("unknown shader use", -1, "rgl", "fatal-to-function");
                break;
        }

        GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 6));
    }

    void update_viewport(const rocket::vec2f_t &size) {
        viewport_size = size;
        glViewport(0, 0, size.x, size.y);
    }
}
