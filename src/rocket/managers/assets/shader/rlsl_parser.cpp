#include "rocket/macros.hpp"
#include <rocket/renderer_helpers.hpp>
#if defined(ROCKETGE__Platform_Android)
    #include <GLES3/gl32.h>
    #include <EGL/egl.h>
#else
    #include <GL/glew.h>
#endif

#include <vulkan/vulkan.h>

#define ROCKETGE__RUNTIME_SKIP_HEADER_INCLUSION
#include <rocket/runtime.hpp>
#include <fstream>
#include <shader.hpp>

namespace rocket {
    static std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return ""; // all whitespace

        size_t end = s.find_last_not_of(" \t\n\r");
        return s.substr(start, end - start + 1);
    }

    static std::string to_hash_version_gl(std::string gl_ver) {
        std::string s;
        int count = 0;
        for (char c : gl_ver) {
            if (count == 4) break;
            if (c >= '0' && c <= '9') {
                s += c;
            }
            ++count;
        }

        return s;
    }

    void rlsl_parse(std::vector<std::string> lines, std::filesystem::path shader_workingdir, rocket::renderer_backend_t backend, void *vk_phys_device) {
        struct rlsl_shader_t {
            std::string version = "unk";
            std::string name = "unk";
            std::string vcode = "";
            std::string fcode = "";
            std::vector<std::string> vk_required_extensions;

            float gl_minimumversion = 0.0;
            float gles_minimumversion = 0.0;
            float vk_minimumversion = 0.0;
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
        auto load_file = [shader_workingdir](std::string path) -> std::vector<std::string> {
            std::ifstream file(shader_workingdir / path);
            if (!file.is_open()) {
                rocket::log("failed to open file path: " + (shader_workingdir / path).string(), "opengl_shader_t", "constructor", "error");
                return {};
            }

            std::vector<std::string> lines;
            std::string line;

            while (std::getline(file, line)) {
                lines.push_back(line);
            }

            return lines;
        };
        constexpr auto split = [](std::string str, char delim) -> std::vector<std::string> {
            std::stringstream ss(str);
            std::string token;
            std::vector<std::string> tokens;
            while (std::getline(ss, token, delim)) {
                tokens.push_back(token);
            }
            return tokens;
        };
        constexpr auto str_tolower = [](std::string str) -> std::string {
            std::transform(str.begin(), str.end(), str.begin(), ::tolower);
            return str;
        };

        /*
GL_MinimumVersion: 3.3
GLES_MinimumVersion: 3.0
VK_MinimumVersion: 1.1
VK_RequireExtension: Something
VK_RequireExtension: Something2
VK_RequireExtension: Something3

//            N bytes of Base64
VK_SPIRV_ShaderB64: 32 FzSUdG ... nVZMlZ1 // and then newline

VertexBegin
...
VertexEnd

FragmentBegin
...
FragmentEnd
        */
        for (size_t i = 0; i < lines.size(); ++i) {
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
                } else if (l.starts_with("GLES_MinimumVersion:")) {
                    rlsl_shader.gles_minimumversion = std::stof(trim(l.substr(20)));
                } else if (l.starts_with("VK_MinimumVersion")) {
                    rlsl_shader.vk_minimumversion = std::stof(trim(l.substr(18)));
                } else if (l.starts_with("VK_RequireExtension:")) {
                    rlsl_shader.vk_required_extensions.push_back(trim(l.substr(20)));
                } else if (l.starts_with("VK_SPIRV_ShaderB64: ")) {
                    std::string formatted_blob = l.substr(20);
                    int _i = 0;
                    std::string size_str;
                    while (formatted_blob[_i] != ' ') {
                        size_str += formatted_blob[_i];
                    }

                    formatted_blob = formatted_blob.substr(0, size_str.size() - 1);
                    size_t size = std::stoull(size_str);
                    put_the_b64_into_array(size);
                }
                else if (l.starts_with("ExternalSource:")) {
                    std::string args = trim(l.substr(15));
                    std::vector<std::string> args_split = split(args, ' ');
                    if (args_split.size() != 2) {
                        rocket::log("issue while parsing RLSL Shader: invalid syntax at line " + std::to_string(ln), "opengl_shader_t", "constructor", "warn");
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
                        rocket::log("issue while parsing RLSL Shader: invalid syntax at line " + std::to_string(ln) + ": " + "unknown shader type '" + type + "'", "opengl_shader_t", "constructor", "warn");
                        continue;
                    }
                } else if (l.starts_with("IncludeHeader:")) {
                    std::vector<std::string> args = split(trim(l.substr(14)), ' ');
                    if (args.size() != 3) {
                        rocket::log("issue while parsing RLSL Shader: invalid syntax at line " + std::to_string(ln) + ": IncludeHeader takes 3 arguments!", "opengl_shader_t", "constructor", "warn");
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

                else if (l.starts_with("VertexStart") || l.starts_with("VertexBegin")) {
                    if (!rlsl_shader.vcode.empty()) {
                        rocket::log("issue while parsing RLSL Shader: invalid syntax at line " + std::to_string(ln) + ": " + "vertex shader already defined, cannot redefine vertex shader", "opengl_shader_t", "constructor", "warn");
                        continue;
                    }

                    std::string gl_ver_string = reinterpret_cast<const char*>(glGetString(GL_VERSION));
                    if (gl_ver_string.starts_with("OpenGL ES")) {
                        rlsl_shader.vcode += "#version " + to_hash_version_gl(std::to_string(rlsl_shader.gles_minimumversion)) + " es\n";
                        rlsl_shader.vcode += "precision highp float;\n";
                    } else
                        rlsl_shader.vcode += "#version " + to_hash_version_gl(std::to_string(rlsl_shader.gl_minimumversion)) + "\n";
                    curmode = mode_t::vertex;
                } else if (l.starts_with("FragmentStart") || l.starts_with("FragmentBegin")) {
                    if (!rlsl_shader.fcode.empty()) {
                        rocket::log("issue while parsing RLSL Shader: invalid syntax at line " + std::to_string(ln) + ": " + "fragment shader already defined, cannot redefine fragment shader", "opengl_shader_t", "constructor", "warn");
                        continue;
                    }
                    std::string gl_ver_string = reinterpret_cast<const char*>(glGetString(GL_VERSION));
                    if (gl_ver_string.starts_with("OpenGL ES")) {
                        rlsl_shader.fcode += "#version " + to_hash_version_gl(std::to_string(rlsl_shader.gles_minimumversion)) + " es\n";
                        rlsl_shader.fcode += "precision highp float;\n";
                    } else
                        rlsl_shader.fcode += "#version " + to_hash_version_gl(std::to_string(rlsl_shader.gl_minimumversion)) + "\n";
                    curmode = mode_t::fragment;
                } // SPIRVBegin() or somthing But thats binary???? idfk

                else if (l.empty()) {
                    continue;
                }
                else {
                    rocket::log("issue while parsing RLSL Shader: invalid syntax at line " + std::to_string(ln) + ": " + "unknown opcode", "opengl_shader_t", "constructor", "warn");
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
            rocket::log("failed to parse RLSL Shader: critical shader code missing", "opengl_shader_t", "constructor", "error");
            return;
        }

        auto vcode_vec = split(rlsl_shader.vcode, '\n');
        std::string constructed_vcode;
        for (size_t i = 0; i < vcode_vec.size(); i++) {
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

        for (size_t i = 0; i < fcode_vec.size(); i++) {
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
            rocket::log("issue while parsing RLSL Shader: shader metadata incomplete", "opengl_shader_t", "constructor", "warn");
        }

        int rlsl_major = std::stoi(split(rlsl_shader.version, '.')[0]);
        int rlsl_minor = std::stoi(split(rlsl_shader.version, '.')[1]);
        if (rlsl_major != ROCKETGE__FEATURE_MAX_RLSL_VERSION_MAJOR 
                || rlsl_minor != ROCKETGE__FEATURE_MAX_RLSL_VERSION_MINOR) {
            rocket::log("This version of RLSL (" + std::to_string(rlsl_major) + "." + std::to_string(rlsl_minor) + " > " + std::to_string(ROCKETGE__FEATURE_MAX_RLSL_VERSION_MAJOR) + "." + std::to_string(ROCKETGE__FEATURE_MAX_RLSL_VERSION_MINOR) + ") is not supported", "opengl_shader_t", "constructor", "warn");
        }

        int major{}, minor{};
        float glver{};
        if (backend == renderer_backend_t::opengl) {
            glGetIntegerv(GL_MAJOR_VERSION, &major);
            glGetIntegerv(GL_MINOR_VERSION, &minor);
        }
        glver = major + minor / 10.f;
        int rlmajor = rlsl_shader.gl_minimumversion;
        int rlminor = (int)((rlsl_shader.gl_minimumversion - rlmajor) * 10 + 0.5f);

        float vkver{};
        int vk_major{};
        int vk_minor{};
        if (backend == renderer_backend_t::vulkan) {
            VkPhysicalDeviceProperties props;
            VkPhysicalDevice dvc = (VkPhysicalDevice) vk_phys_device;
            vkGetPhysicalDeviceProperties(dvc, &props);

            uint32_t ver = props.apiVersion;
            vk_major = VK_VERSION_MAJOR(ver);
            vk_minor = VK_VERSION_MINOR(ver);
            vkver = major + (minor / 10.f);
        }

        if (backend == renderer_backend_t::opengl) {
            if (rlsl_shader.gles_minimumversion > 0.0) {
                if (glver < rlsl_shader.gles_minimumversion) {
                    rocket::log("issue while parsing RLSL Shader: Loaded OpenGL ES Version lower than Minimum OpenGL ES Version (" + std::to_string(major) + "." + std::to_string(minor) + " < " + std::to_string(rlmajor) + "." + std::to_string(rlminor) + + ")", "opengl_shader_t", "constructor", "warn");
                }
            } else if (glver < rlsl_shader.gl_minimumversion) {
                rocket::log("issue while parsing RLSL Shader: Loaded OpenGL Version lower than Minimum OpenGL Version (" + std::to_string(major) + "." + std::to_string(minor) + " < " + std::to_string(rlmajor) + "." + std::to_string(rlminor) + + ")", "opengl_shader_t", "constructor", "warn");
            }
        } else if (backend == renderer_backend_t::vulkan) {
            if (vkver < rlsl_shader.vk_minimumversion) {
                rocket::log("issue while parsing RLSL Shader: Loaded Vulkan Version lower than Minimum Vulkan Version (" + std::to_string(vk_major) + "." + std::to_string(vk_minor) + ")", "opengl_shader_t", "constructor", "warn");
            }
        } else if (backend == renderer_backend_t::null) {
        } else {
        }
    }
}
