#include <GL/glew.h>
#include <iostream>
#include "../../include/rocket/shader.hpp"
#include "../../include/rocket/runtime.hpp"
#include "util.hpp"

namespace rocket {
    void gl_check_errors(int step) {
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cerr << "[OpenGL Error] Step " << step << ": " << std::hex << err << std::dec << std::endl;
        }
    }

    bool check_shader_compile(GLuint shader, const char* shader_type) {
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[1024];
            glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            std::cerr << shader_type << " shader compilation error:\n" << infoLog << std::endl;
            return false;
        }
        return true;
    }

    bool check_program_link(GLuint program) {
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[1024];
            glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
            std::cerr << "Program linking error:\n" << infoLog << std::endl;
            return false;
        }
        return true;
    }
        
    shader_t::shader_t(shader_type type, std::string vcode, std::string fcode) : type(type), fcode(fcode), vcode(vcode) {
        this->glshaderv = glCreateShader(GL_VERTEX_SHADER);
        this->glshaderf = glCreateShader(GL_FRAGMENT_SHADER);

        const char* vsrc = vcode.c_str();
        const char* fsrc = fcode.c_str();

        while (glGetError() != GL_NO_ERROR) {}
        glShaderSource(glshaderv, 1, &vsrc, nullptr);
        glCompileShader(glshaderv);
        if (!check_shader_compile(glshaderv, "Vertex")) {
            throw std::runtime_error("Vertex shader compilation failed");
        }
        gl_check_errors(1);

        glShaderSource(glshaderf, 1, &fsrc, nullptr);
        glCompileShader(glshaderf);
        if (!check_shader_compile(glshaderf, "Fragment")) {
            throw std::runtime_error("Fragment shader compilation failed");
        }
        gl_check_errors(2);

        glprogram = glCreateProgram();
        glAttachShader(glprogram, glshaderv);
        gl_check_errors(3);

        glAttachShader(glprogram, glshaderf);
        gl_check_errors(4);

        glLinkProgram(glprogram);
        if (!check_program_link(glprogram)) {
            throw std::runtime_error("Shader program linking failed");
        }
        gl_check_errors(5);

        glDeleteShader(glshaderv);
        gl_check_errors(6);
        glDeleteShader(glshaderf);
        gl_check_errors(7);

        rocket::log("(2) Shaders compiled successfully", "shader_t", "constructor", "info");
    }

    GLint getloc(std::string name, GLuint glprogram) {
        return glGetUniformLocation(glprogram, name.c_str());
    }

    void shader_t::set_uniform(std::string name, float value) {
        GLint loc = getloc(name, glprogram);
        glUniform1f(loc, value);
    }

    void shader_t::set_uniform(std::string name, int value) {
        GLint loc = getloc(name, glprogram);
        glUniform1i(loc, value);
    }

    void shader_t::set_uniform(std::string name, vec2f_t value) {
        GLint loc = getloc(name, glprogram);
        glUniform2f(loc, value.x, value.y);
    }

    void shader_t::set_uniform(std::string name, vec3f_t value) {
        GLint loc = getloc(name, glprogram);
        glUniform3f(loc, value.x, value.y, value.z);
    }

    void shader_t::set_uniform(std::string name, vec4f_t value) {
        GLint loc = getloc(name, glprogram);
        glUniform4f(loc, value.x, value.y, value.z, value.w);
    }

    void shader_t::set_uniform(std::string name, mat4 value) {
        GLint loc = getloc(name, glprogram);
        glUniformMatrix4fv(loc, 1, GL_FALSE, &value.columns[0][0]);
    }

    void shader_t::set_uniform_raw(std::string name, GLenum type, const void* data, GLsizei count) {
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
            // Add other types as needed
            default:
                // unsupported type
                break;
        }
    }

    shader_t shader_t::rectangle(rgba_color fill_color) {
        std::string vcode = R"(

// Vertex shader
#version 330 core
layout(location = 0) in vec2 aPos;
uniform mat4 u_transform;

void main() {
    gl_Position = u_transform * vec4(aPos, 0.0, 1.0);
}

    )";

        std::string fcode = R"(
// Fragment shader
#version 330 core
out vec4 FragColor;
uniform vec4 u_color;

void main() {
    FragColor = u_color;
}

    )";

        shader_t shader(shader_type::vert_frag, vcode, fcode);
        shader.set_uniform("u_color", vec4<float>(fill_color.x / 255.f, fill_color.y / 255.f, fill_color.z / 255.f, fill_color.w / 255.f));

        return shader;
    }

    shader_t::~shader_t() {}
}
