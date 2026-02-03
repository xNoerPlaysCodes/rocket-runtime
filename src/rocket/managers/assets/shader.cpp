#include <GL/glew.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include "../../include/rocket/shader.hpp"
#include "../../include/rocket/runtime.hpp"
#include "rgl.hpp"
#include "util.hpp"

namespace rocket {
    void gl_check_errors(int step) {
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            rocket::log_error("OpenGL error at step " + std::to_string(step), "OpenGL", "warning");
        }
    }

    bool check_shader_compile(GLuint shader, const char* shader_type) {
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[1024];
            glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            rocket::log_error(std::string(shader_type) + " " + "compilation error", "OpenGL::ShaderCompiler", "error");
            rocket::log_error(std::string(infoLog), "OpenGL::ShaderCompiler", "error");
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
            rocket::log_error("link error", "OpenGL::ShaderLinker", "error");
            rocket::log_error(std::string(infoLog), "OpenGL::ShaderLinker", "error");
            return false;
        }
        return true;
    }

    void shader_t::shader_init() {
        this->glshaderv = glCreateShader(GL_VERTEX_SHADER);
        this->glshaderf = glCreateShader(GL_FRAGMENT_SHADER);

        const char* vsrc = vcode.c_str();
        const char* fsrc = fcode.c_str();

        const char *default_vcode = R"(
#version 330 core

layout(location = 0) in vec2 aPos;
out vec2 fragPos;

void main() {
    fragPos = aPos;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
            )";
        const char *default_fcode = R"(
            #version 330 core

in vec2 fragPos;
out vec4 FragColor;

void main() {
    FragColor = vec4(1., 0., 1., 1.);
}
            )";

        while (glGetError() != GL_NO_ERROR) {}
        glShaderSource(glshaderv, 1, &vsrc, nullptr);
        glCompileShader(glshaderv);
        if (!check_shader_compile(glshaderv, "Vertex")) {
            rocket::log_error("Cannot continue with failed compilation", "shader_t::constructor", "error");
            this->glprogram = rgl::nocache_compile_shader(default_vcode, default_fcode);
            this->fcode = default_fcode;
            this->vcode = default_vcode;
            return;
        }
        gl_check_errors(1);

        glShaderSource(glshaderf, 1, &fsrc, nullptr);
        glCompileShader(glshaderf);
        if (!check_shader_compile(glshaderf, "Fragment")) {
            rocket::log_error("Cannot continue with failed compilation", "shader_t::constructor", "error");
            this->glprogram = rgl::nocache_compile_shader(default_vcode, default_fcode);
            this->fcode = default_fcode;
            this->vcode = default_vcode;
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
            rocket::log_error("Cannot continue with failed linking", "shader_t::constructor", "error");
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

    std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return ""; // all whitespace

        size_t end = s.find_last_not_of(" \t\n\r");
        return s.substr(start, end - start + 1);
    }
        
    shader_t::shader_t(shader_type type, std::string vcode, std::string fcode, std::string name) : type(type), vcode(vcode), fcode(fcode), name(name) {
        this->shader_init();
    }

    void shader_t::parse(const std::vector<std::string> &lines, std::filesystem::path shader_workingdir) {
        struct rlsl_shader_t {
            std::string version = "unk";
            std::string name = "unk";
            std::string vcode = "";
            std::string fcode = "";

            float gl_minimumversion = 4.3;
        };
        enum class mode_t {
            rlsl,
            vertex,
            fragment
        };
        struct inserted_header_t {
            int at_line = 0;
            std::string at_type = "";
            std::string insert_code = "";
        };
        std::vector<inserted_header_t> inserted_headers;
        rlsl_shader_t rlsl_shader;
        mode_t curmode = mode_t::rlsl;
        static auto load_file = [&shader_workingdir](std::string path) -> std::vector<std::string> {
            std::ifstream file(shader_workingdir / path);
            if (!file.is_open()) {
                rocket::log_error("failed to open file path: " + (shader_workingdir / path).string(), "shader_t::constructor", "error");
                return {};
            }

            std::vector<std::string> lines;
            std::string line;

            while (std::getline(file, line)) {
                lines.push_back(line);
            }

            return lines;
        };
        static auto split = [](std::string str, char delim) -> std::vector<std::string> {
            std::stringstream ss(str);
            std::string token;
            std::vector<std::string> tokens;
            while (std::getline(ss, token, delim)) {
                tokens.push_back(token);
            }
            return tokens;
        };
        static auto str_tolower = [](std::string str) -> std::string {
            std::transform(str.begin(), str.end(), str.begin(), ::tolower);
            return str;
        };
        for (int i = 0; i < lines.size(); ++i) {
            const std::string &l = lines[i];
            int ln = i + 1;
            if (l.starts_with("//")) {
                continue;
            }
            if (curmode == mode_t::rlsl) {
                if (l.starts_with("Version:")) {
                    rlsl_shader.version = trim(l.substr(8));
                } else if (l.starts_with("Name:")) {
                    rlsl_shader.name = trim(l.substr(5));
                } else if (l.starts_with("GL_MinimumVersion:")) {
                    rlsl_shader.gl_minimumversion = std::stof(trim(l.substr(18)));
                } else if (l.starts_with("ExternalSource:")) {
                    std::string args = trim(l.substr(15));
                    std::vector<std::string> args_split = split(args, ' ');
                    if (args_split.size() != 2) {
                        rocket::log_error("issue while parsing RLSL Shader: invalid syntax at line " + std::to_string(ln), "shader_t::constructor", "warn");
                        continue;
                    }

                    std::string type = str_tolower(args_split[0]);
                    std::string path = args_split[1];

                    std::vector<std::string> lines = load_file(path);

                    if (type == "vertex") {
                        for (auto &l : lines) {
                            rlsl_shader.vcode += l + "\n";
                        }
                    } else if (type == "fragment") {
                        for (auto &l : lines) {
                            rlsl_shader.fcode += l + "\n";
                        }
                    }
                    else {
                        rocket::log_error("issue while parsing RLSL Shader: invalid syntax at line " + std::to_string(ln) + ": " + "unknown shader type '" + type + "'", "shader_t::constructor", "warn");
                        continue;
                    }
                } else if (l.starts_with("IncludeHeader:")) {
                    std::vector<std::string> args = split(trim(l.substr(14)), ' ');
                    if (args.size() != 3) {
                        rocket::log_error("issue while parsing RLSL Shader: invalid syntax at line " + std::to_string(ln) + ": IncludeHeader takes 3 arguments!", "shader_t::constructor", "warn");
                        continue;
                    }

                    int insert_linenum = std::stoi(args[0]);
                    std::string insert_shadertype = str_tolower(args[1]);
                    std::string insert_path = args[2];

                    inserted_header_t header;
                    header.at_type = insert_shadertype;
                    header.at_line = insert_linenum;
                    std::vector<std::string> lines = load_file(insert_path);
                    for (auto &l : lines) {
                        header.insert_code += l + "\n";
                    }

                    inserted_headers.push_back(header);
                }

                else if (l.starts_with("VertexStart")) {
                    if (!rlsl_shader.vcode.empty()) {
                        rocket::log_error("issue while parsing RLSL Shader: invalid syntax at line " + std::to_string(ln) + ": " + "vertex shader already defined, cannot redefine vertex shader", "shader_t::constructor", "warn");
                        continue;
                    }
                    curmode = mode_t::vertex;
                } else if (l.starts_with("FragmentStart")) {
                    if (!rlsl_shader.fcode.empty()) {
                        rocket::log_error("issue while parsing RLSL Shader: invalid syntax at line " + std::to_string(ln) + ": " + "fragment shader already defined, cannot redefine fragment shader", "shader_t::constructor", "warn");
                        continue;
                    }
                    curmode = mode_t::fragment;
                }

                else if (l.empty()) {
                    continue;
                }
                else {
                    rocket::log_error("issue while parsing RLSL Shader: invalid syntax at line " + std::to_string(ln) + ": " + "unknown opcode", "shader_t::constructor", "warn");
                }
            }
            else if (curmode == mode_t::vertex) {
                if (l.starts_with("VertexEnd")) {
                    curmode = mode_t::rlsl;
                } else {
                    rlsl_shader.vcode += l + "\n";
                }
            } else if (curmode == mode_t::fragment) {
                if (l.starts_with("FragmentEnd")) {
                    curmode = mode_t::rlsl;
                } else {
                    rlsl_shader.fcode += l + "\n";
                }
            }
        }

        if (rlsl_shader.vcode.empty() || rlsl_shader.fcode.empty()) {
            rocket::log_error("failed to parse RLSL Shader: critical shader code missing", "shader_t::constructor", "error");
            return;
        }

        auto vcode_vec = split(rlsl_shader.vcode, '\n');
        std::string constructed_vcode;
        for (int i = 0; i < vcode_vec.size(); i++) {
            int linenum = i + 1;
            std::string &l = vcode_vec.at(i);
            bool insert = true;

            for (auto &h : inserted_headers) {
                if (h.at_type == "vertex" && h.at_line == linenum) {
                    constructed_vcode += l + "\n";
                    constructed_vcode += h.insert_code;

                    insert = false;
                }
            }

            if (insert) {
                constructed_vcode += l + "\n";
            }
        }
        rlsl_shader.vcode = constructed_vcode;

        auto fcode_vec = split(rlsl_shader.fcode, '\n');
        std::string constructed_fcode;
        for (int i = 0; i < fcode_vec.size(); i++) {
            int linenum = i + 1;
            std::string &l = fcode_vec.at(i);
            bool insert = true;
            for (auto &h : inserted_headers) {
                if (h.at_type == "fragment" && h.at_line == linenum) {
                    constructed_fcode += l;
                    constructed_fcode += "\n";
                    constructed_fcode += h.insert_code;

                    insert = false;
                }
            }
            if (insert) {
                constructed_fcode += l + "\n";
            }
        }
        rlsl_shader.fcode = constructed_fcode;

        if (rlsl_shader.version == "unk" || rlsl_shader.name == "unk") {
            rocket::log_error("issue while parsing RLSL Shader: shader metadata incomplete", "shader_t::constructor", "warn");
        }

        int major, minor;
        float glver;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
        glver = major + minor / 10.f;
        int rlmajor = rlsl_shader.gl_minimumversion;
        int rlminor = (int)((rlsl_shader.gl_minimumversion - rlmajor) * 10 + 0.5f);
        int rlsl_major = std::stoi(split(rlsl_shader.version, '.')[0]);
        int rlsl_minor = std::stoi(split(rlsl_shader.version, '.')[1]);
        if (rlsl_major != ROCKETGE__FEATURE_MAX_RLSL_VERSION_MAJOR 
                || rlsl_minor != ROCKETGE__FEATURE_MAX_RLSL_VERSION_MINOR) {
            rocket::log_error("This version of RLSL (" + std::to_string(rlsl_major) + "." + std::to_string(rlsl_minor) + " > " + std::to_string(ROCKETGE__FEATURE_MAX_RLSL_VERSION_MAJOR) + "." + std::to_string(ROCKETGE__FEATURE_MAX_RLSL_VERSION_MINOR) + ") is not supported", "shader_t::constructor", "warn");
        }
        if (glver < rlsl_shader.gl_minimumversion) {
            rocket::log_error("issue while parsing RLSL Shader: Loaded OpenGL Version lower than Minimum OpenGL Version (" + std::to_string(major) + "." + std::to_string(minor) + " < " + std::to_string(rlmajor) + "." + std::to_string(rlminor) + + ")", "shader_t::constructor", "warn");
        }

        this->vcode = rlsl_shader.vcode;
        this->fcode = rlsl_shader.fcode;
        this->name = rlsl_shader.name;
        this->rlsl_version = rlsl_shader.version;

        this->shader_init();

        rocket::log("Loaded RLSL Shader '" + rlsl_shader.name + "'", "shader_t", "constructor", "info");
    }

    shader_t::shader_t(shader_type type, std::string rlsl_shader_path) {
        this->type = type;
        std::ifstream istream(rlsl_shader_path);
        std::filesystem::path shader_workingdir = std::filesystem::path(rlsl_shader_path).parent_path();
        if (!istream.is_open()) {
            rocket::log_error("failed to open file path: " + rlsl_shader_path, "shader_t::shader_t(shader_type, std::string)", "error");
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

    shader_t::shader_t() {}

    shader_t shader_t::load_from_rlsl_source(shader_type type, std::string rlsl) {
        shader_t shader;
        shader.type = type;

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

        shader.parse(split(rlsl, '\n'), shader_workingdir);
        return shader;
    }

    GLint getloc(std::string name, GLuint glprogram) {
        GLint loc = glGetUniformLocation(glprogram, name.c_str());
        if (loc == -1) {
            std::cerr << "Uniform " << name << " not found in shader" << std::endl;
            rocket::exit(45);
        }
        glUseProgram(glprogram);
        return loc;
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
