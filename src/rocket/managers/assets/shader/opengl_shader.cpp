#include "rocket/macros.hpp"
#if defined(ROCKETGE__Platform_Android)
    #include <GLES3/gl32.h>
    #include <EGL/egl.h>
#else
    #include <lib/glad/glad.h>
#endif
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include "../../include/rocket/shader.hpp"
#include "../../include/rocket/runtime.hpp"
#include "rgl.hpp"
#include "util.hpp"
#include <shader.hpp>

namespace rocket {
    static void gl_check_errors(int step) {
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            rocket::log("OpenGL error at step " + std::to_string(step), "OpenGL", "glGetError", "warning");
        }
    }

    static bool check_shader_compile(GLuint shader, const char* shader_type) {
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[1024];
            glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            rocket::log(std::string(shader_type) + " " + "compilation error", "OpenGL", "ShaderCompiler", "error");
            rocket::log(std::string(infoLog), "OpenGL", "ShaderCompiler", "error");
            return false;
        }
        return true;
    }

    static bool check_program_link(GLuint program) {
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[1024];
            glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
            rocket::log("link error", "OpenGL", "ShaderLinker", "error");
            rocket::log(std::string(infoLog), "OpenGL", "ShaderLinker", "error");
            return false;
        }
        return true;
    }

    void opengl_shader_t::shader_init() {
        this->glshaderv = glCreateShader(GL_VERTEX_SHADER);
        this->glshaderf = glCreateShader(GL_FRAGMENT_SHADER);

        const char* vsrc = vcode.c_str();
        const char* fsrc = fcode.c_str();

        const char *default_vcode = R"(
#version 300 es
precision highp float;

layout(location = 0) in vec2 aPos;
out vec2 fragPos;

void main() {
    fragPos = aPos;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
            )";
        const char *default_fcode = R"(
#version 300 es
precision highp float;

in vec2 fragPos;
out vec4 FragColor;

void main() {
    FragColor = vec4(0.);
}
            )";

        while (glGetError() != GL_NO_ERROR) {}
        glShaderSource(glshaderv, 1, &vsrc, nullptr);
        glCompileShader(glshaderv);
        if (!check_shader_compile(glshaderv, "Vertex")) {
            rocket::log("Cannot continue with failed compilation", "opengl_shader_t", "constructor", "error");
            this->glprogram = rgl::nocache_compile_shader(default_vcode, default_fcode);
            return;
        }
        gl_check_errors(1);

        glShaderSource(glshaderf, 1, &fsrc, nullptr);
        glCompileShader(glshaderf);
        if (!check_shader_compile(glshaderf, "Fragment")) {
            rocket::log("Cannot continue with failed compilation", "opengl_shader_t", "constructor", "error");
            this->glprogram = rgl::nocache_compile_shader(default_vcode, default_fcode);
            return;
        }
        gl_check_errors(2);

        glprogram = glCreateProgram();
        glAttachShader(glprogram, glshaderv);
        gl_check_errors(3);

        glAttachShader(glprogram, glshaderf);
        gl_check_errors(4);

        glLinkProgram(glprogram);
        if (!check_program_link(glprogram)) {
            rocket::log("Cannot continue with failed linking", "opengl_shader_t", "constructor", "error");
            return;
        }
        gl_check_errors(5);

        glDeleteShader(glshaderv);
        gl_check_errors(6);
        glDeleteShader(glshaderf);
        gl_check_errors(7);

        std::array<float, 12> vertices = {
            -1.0f, -1.0f,   // bottom left
             1.0f, -1.0f,   // bottom right
            -1.0f,  1.0f,   // top left

            -1.0f,  1.0f,   // top left
             1.0f, -1.0f,   // bottom right
             1.0f,  1.0f    // top right
        };
        glGenVertexArrays(1, &vao);
        gl_check_errors(8);
        glGenBuffers(1, &vbo);
        gl_check_errors(9);

        glBindVertexArray(vao);
        gl_check_errors(10);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        gl_check_errors(11);

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);
        gl_check_errors(12);

        // This matches "layout(location = 0) in vec2 aPos;" in vertex shader
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        gl_check_errors(13);
        glEnableVertexAttribArray(0);
        gl_check_errors(14);

        glBindVertexArray(0);
        gl_check_errors(15);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        gl_check_errors(16);
    }

    opengl_shader_t::opengl_shader_t(shader_type type, std::string vcode, std::string fcode, std::string name) {
        this->type = type;
        this->vcode = vcode;
        this->fcode = fcode;
        this->name = name;

        this->shader_init();
    }

    void opengl_shader_t::parse(const std::vector<std::string> &lines, std::filesystem::path shader_workingdir) {
        rlsl_parsed_result_t res = rocket::rlsl_parse(lines, shader_workingdir, util::get_renderer_backend(util::get_global_renderer_2d()), nullptr);

        this->vcode = res.gl_vert_code;
        this->fcode = res.gl_frag_code;
        this->name = res.name;
        this->rlsl_version = res.rlsl_version;

        this->shader_init();

        rocket::log("Loaded RLSL Shader: " + res.name, "opengl_shader_t", "constructor", "info");
    }

    opengl_shader_t::opengl_shader_t(shader_type type, std::filesystem::path rlsl_shader_path) {
        this->type = type;
        std::ifstream istream(rlsl_shader_path);
        std::filesystem::path shader_workingdir = rlsl_shader_path.parent_path();
        if (!istream.is_open()) {
            rocket::log("failed to open file path: " + rlsl_shader_path.string(), "opengl_shader_t::opengl_shader_t(shader_type, std", "string)", "error");
            return;
        }

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(istream, line)) {
            lines.push_back(line);
        }

        istream.close();

        this->parse(lines, shader_workingdir);
    }

    opengl_shader_t::opengl_shader_t() {}

    opengl_shader_t::opengl_shader_t(shader_type type, const std::string &rlsl) {
        this->type = type;

        static auto split = [](std::string str, char delim) -> std::vector<std::string> {
            std::stringstream ss(str);
            std::string token;
            std::vector<std::string> tokens;
            while (std::getline(ss, token, delim)) {
                tokens.push_back(token);
            }
            return tokens;
        };

        std::filesystem::path shader_workingdir = std::filesystem::current_path();

        this->parse(split(rlsl, '\n'), shader_workingdir);
    }

    static GLint getloc(std::string name, GLuint glprogram) {
        GLint loc = glGetUniformLocation(glprogram, name.c_str());
        if (loc == -1) {
            return -1;
        }
        glUseProgram(glprogram);
        return loc;
    }

    void opengl_shader_t::set_parameter(std::string name, float value) {
        GLint loc = getloc(name, glprogram);
        glUniform1f(loc, value);
    }

    void opengl_shader_t::set_parameter(std::string name, int value) {
        GLint loc = getloc(name, glprogram);
        glUniform1i(loc, value);
    }

    void opengl_shader_t::set_parameter(std::string name, vec2f_t value) {
        GLint loc = getloc(name, glprogram);
        glUniform2f(loc, value.x, value.y);
    }

    void opengl_shader_t::set_parameter(std::string name, vec3f_t value) {
        GLint loc = getloc(name, glprogram);
        glUniform3f(loc, value.x, value.y, value.z);
    }

    void opengl_shader_t::set_parameter(std::string name, vec4f_t value) {
        GLint loc = getloc(name, glprogram);
        glUniform4f(loc, value.x, value.y, value.z, value.w);
    }

    void opengl_shader_t::set_parameter(std::string name, mat4 value) {
        GLint loc = getloc(name, glprogram);
        glUniformMatrix4fv(loc, 1, GL_FALSE, &value.columns[0][0]);
    }

    void opengl_shader_t::set_parameter_raw(std::string name, GLenum type, const void* data, GLsizei count) {
        GLint location = glGetUniformLocation(glprogram, name.c_str());
        if (location == -1) return; // uniform not found

        switch (type) {
            case GL_FLOAT:
                glUniform1fv(location, count, (const GLfloat*)data);
                break;
            case GL_FLOAT_VEC2:
                glUniform2fv(location, count / 2, (const GLfloat*)data);
                break;
            case GL_FLOAT_VEC3:
                glUniform3fv(location, count / 3, (const GLfloat*)data);
                break;
            case GL_FLOAT_VEC4:
                glUniform4fv(location, count / 4, (const GLfloat*)data);
                break;
            case GL_FLOAT_MAT2:
                glUniformMatrix2fv(location, count / 4, GL_FALSE, (const GLfloat*)data);
                break;
            case GL_FLOAT_MAT3:
                glUniformMatrix3fv(location, count / 9, GL_FALSE, (const GLfloat*)data);
                break;
            case GL_FLOAT_MAT4:
                glUniformMatrix4fv(location, count / 16, GL_FALSE, (const GLfloat*)data);
                break;
            case GL_INT:
                glUniform1iv(location, count, (const GLint*)data);
                break;
            case GL_INT_VEC2:
                glUniform2iv(location, count / 2, (const GLint*)data);
                break;
            case GL_INT_VEC3:
                glUniform3iv(location, count / 3, (const GLint*)data);
                break;
            case GL_INT_VEC4:
                glUniform4iv(location, count / 4, (const GLint*)data);
                break;
            default:
                rocket::log("unsupported shader parameter type", "opengl_shader_t", "set_parameter_raw", "error");
                break;
        }
    }

    bool opengl_shader_t::operator==(const shader_i &other) const {
        if (!rocket::instance_of<const opengl_shader_t>(&other)) {
            rocket::log("Objects are of unequal type", "opengl_shader_t", "operator==", "error");
            return false;
        }
        return this->glprogram == dynamic_cast<const opengl_shader_t*>(&other)->glprogram;
    }

    opengl_shader_t::~opengl_shader_t() {
    }
}
