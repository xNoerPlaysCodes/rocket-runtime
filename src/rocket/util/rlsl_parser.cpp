#include "rocket/macros.hpp"
#include <algorithm>
#include <iostream>
#include <rocket/renderer_helpers.hpp>
#if defined(ROCKETGE__Platform_Android)
    #include <GLES3/gl32.h>
    #include <EGL/egl.h>
#else
    #include <lib/glad/glad.h>
#endif

#include <vulkan/vulkan.h>

#define ROCKETGE__RUNTIME_SKIP_HEADER_INCLUSION
#include <rocket/runtime.hpp>
#include <fstream>
#include <shader.hpp>
#include <util.hpp>
#include <intl_macros.hpp>
#include "lib/base64/base64.h"

namespace rocket {
    namespace {
        std::string trim(const std::string& s) {
            size_t start = s.find_first_not_of(" \t\n\r");
            if (start == std::string::npos) return ""; // all whitespace

            size_t end = s.find_last_not_of(" \t\n\r");
            return s.substr(start, end - start + 1);
        }

        std::string to_hash_version_gl(std::string gl_ver) {
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

        bool parse_formatted_b64_blob(const std::string &formatted_blob, std::vector<uint8_t> &bytes) {
            bytes.clear();

            std::size_t digit_count = 0;
            while (digit_count < formatted_blob.size() &&
                   formatted_blob[digit_count] >= '0' &&
                   formatted_blob[digit_count] <= '9') {
                ++digit_count;
            }

            if (digit_count == 0) {
                return false;
            }

            if (digit_count >= formatted_blob.size()) {
                return false;
            }

            if (formatted_blob[digit_count] != ' ') {
                return false;
            }

            std::size_t expected_length = 0;
            try {
                expected_length = std::stoull(formatted_blob.substr(0, digit_count));
            } catch (...) {
                return false;
            }

            std::string blob = formatted_blob.substr(digit_count + 1);

            std::vector<std::uint8_t> decoded(expected_length);
            const std::size_t written = b64_decode(
                reinterpret_cast<std::uint8_t *>(blob.data()),
                blob.size(),
                decoded.data());

            if (written != expected_length) {
                return false;
            }

            bytes = std::move(decoded);
            return true;
        }

        std::vector<std::string> split_delim_str(const std::string &s, std::string_view delim) {
            std::vector<std::string> out;

            if (delim.empty()) {
                out.push_back(s);
                return out;
            }

            size_t start = 0;

            while (true) {
                size_t pos = s.find(delim, start);

                if (pos == std::string::npos) {
                    out.emplace_back(s.substr(start));
                    break;
                }

                out.emplace_back(s.substr(start, pos - start));
                start = pos + delim.size();
            }

            return out;
        }

        std::string parse_str(const std::string &raw) {
            std::string s;
            s.reserve(raw.size());
            bool escape = false;
            // if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"') {
            //     s = raw.substr(1, raw.size() - 2);
            // }
            for (size_t i = 0; i < raw.size(); ++i) {
                if ((i == 0 || i == raw.size() - 1) && raw[i] == '"')
                    continue;
                if (raw[i] == '\\') {
                    escape = true;
                    continue;
                }

                char push_c = '\0';

                if (escape) {
                    escape = false;
                    switch (raw[i]) {
                        case '\\':
                            push_c = '\\'; break;
                        case '"':
                            push_c = '"'; break;
                        case 'n':
                            push_c = '\n'; break;
                        case 't':
                            push_c = '\t'; break;
                        case 'r':
                            push_c = '\r'; break;
                        default:
                            push_c = raw[i]; break;
                    }
                } else {
                    push_c = raw[i];
                }

                if (push_c != '\0')
                    s += push_c;
            }

            if (escape)
                s += '\\';

            return s;
        }
    }

    struct rlsl_shader_t {
        std::string version = "unk";
        std::string name = "unk";
        std::string vcode = "";
        std::string fcode = "";
        std::vector<std::string> vk_required_extensions;

        std::vector<uint8_t> vdata;
        std::vector<uint8_t> fdata;

        int gl_minimumversion = 0;
        int gles_minimumversion = 0;
        int vk_minimumversion = 0;
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

    rlsl_parsed_result_t rlsl_parse(std::vector<std::string> lines, std::filesystem::path shader_workingdir, rocket::renderer_backend_t backend, void *vk_phys_device) {
        std::vector<inserted_header_t> inserted_headers;
        rlsl_shader_t rlsl_shader;
        auto load_file = [shader_workingdir](std::string path) -> std::vector<std::string> {
            std::ifstream file(shader_workingdir / path);
            if (!file.is_open()) {
                rocket::log("failed to open file path: " + (shader_workingdir / path).string(), "shader_i", "rlsl_parser", "error");
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

        struct {
            bool no_property_override = false;
        } lang_properties;

        struct {
            int errors = 0;
            int warnings = 0;
            std::vector<std::string> namespaces;
            std::unordered_map<std::string, std::string> properties;
            std::unordered_map<std::string, std::vector<std::string>> properties_list;
            mode_t parser_mode = mode_t::rlsl;
        } state;

        const auto set_property = [&state](const std::string &s, const std::string &v) -> void {
            state.properties[s] = v;
        };

        const auto add_property = [&state](const std::string &s, const std::string &v) -> void {
            state.properties_list[s].push_back(v);
        };

        const auto namespace_to_str = [&state]() -> std::string {
            std::string namespace_str;
            for (const auto &nm : state.namespaces) {
                namespace_str += nm + ".";
            }

            if (namespace_str.size() > 0)
                namespace_str = namespace_str.substr(0, namespace_str.size() - 1);

            return namespace_str;
        };

        const auto has_property = [&state](const std::string &s) -> bool {
            return state.properties.find(s) != state.properties.end();
        };

        const auto has_property_ls = [&state](const std::string& list_name, const std::string& value = "__any") -> bool {
            auto it = state.properties_list.find(list_name);
            if (it == state.properties_list.end()) {
                return false;
            }

            if (value == "__any") {
                return true;
            }

            return std::find(it->second.begin(), it->second.end(), value) != it->second.end();
        };

        const auto get_property = [&state, &has_property](const std::string &s) -> std::string {
            return state.properties[s];
        };

        const auto get_property_ls = [&state, &has_property_ls](const std::string& list_name) -> std::vector<std::string> {
            return state.properties_list[list_name];
        };

        for (size_t i = 0; i < lines.size(); ++i) {
            const std::string &raw_line = lines[i];
            int ln = i + 1;
            if (raw_line.starts_with("//")) {
                continue;
            }
            std::string l = trim(raw_line);
            std::vector<std::string> parts = split(l, ' ');
#define PS_ERROR(LOG) rocket::log(LOG, "util", "rlsl_parser", "error"); state.errors++; continue
#define PS_WARN(LOG) rocket::log(LOG, "util", "rlsl_parser", "warn"); state.warnings++ 
#define PS_UNKNOWN_KW(kw) PS_ERROR(std::string("unknown keyword ") + kw)
#define PS_UNKNOWN_VALUE(vl) PS_ERROR(std::string("unknown value ") + vl)
            if (parts.size() == 0) {
                continue;
            }
            const std::string &kw = parts.at(0).substr(1);
            if (state.parser_mode == mode_t::rlsl) {
                if (kw == "LangProperty") {
                    const std::string &property = trim(parts.at(1));
                    const std::string &value = trim(parts.at(2));
                    if (property == "NoPropertyOverride") {
                        if (value == "true") {
                            lang_properties.no_property_override = true;
                        } else if (value == "false") {
                            lang_properties.no_property_override = false;
                        } else {
                            PS_UNKNOWN_VALUE(value);
                        }
                    }
                }
                else if (kw == "Set") {
                    const std::string &given_property = trim(parts.at(1));
                    const std::string &given_value = trim(parts.at(2));

                    std::string value = parse_str(given_value);

                    std::string property = namespace_to_str();
                    if (!property.empty())
                        property += ".";
                    property += given_property;

                    if (has_property(property)) {
                        std::string err_msg = "property '" + property + "' already exists with value: '" + get_property(property) + "', overriding";
                        if (lang_properties.no_property_override) {
                            PS_WARN(err_msg);
                        } else {
                            PS_ERROR(err_msg);
                        }
                    }

                    set_property(property, value);
                }
                else if (kw == "EnterNamespace") {
                    const std::string &given_namespace = trim(parts.at(1));
                    state.namespaces.push_back(given_namespace);
                }
                else if (kw == "Add") {
                    const std::string &given_property = trim(parts.at(1));
                    const std::string &given_value = trim(parts.at(2));

                    std::string value = parse_str(given_value);

                    std::string property = namespace_to_str();
                    if (!property.empty())
                        property += ".";
                    property += given_property;

                    add_property(property, value);
                }
                else if (kw == "EnterNamespace") {
                    const std::string &given_namespace = trim(parts.at(1));
                    state.namespaces.push_back(given_namespace);
                }
                else if (kw == "ExitNamespace") {
                    if (state.namespaces.size() == 0) {
                        PS_ERROR("extraneous 'Exit' statement");
                    } else {
                        state.namespaces.pop_back();
                    }
                }
                else if (kw == "Begin") {
                    const std::string &mode = trim(parts.at(1));
                    if (mode == "VertexShader") {
                        state.parser_mode = mode_t::vertex;
                    } else if (mode == "FragmentShader") {
                        state.parser_mode = mode_t::fragment;
                    } else {
                        PS_UNKNOWN_VALUE(mode);
                    }
                }
                else {
                    PS_UNKNOWN_KW(kw);
                }
            } else if (state.parser_mode == mode_t::vertex) {
                if (kw == "End") {
                    state.parser_mode = mode_t::rlsl;
                } else {
                    rlsl_shader.vcode += raw_line + "\n";
                }
            } else if (state.parser_mode == mode_t::fragment) {
                if (kw == "End") {
                    state.parser_mode = mode_t::rlsl;
                } else {
                    rlsl_shader.fcode += raw_line + "\n";
                }
            }
        }

        if ((rlsl_shader.vcode.empty() || rlsl_shader.fcode.empty()) && backend == renderer_backend_t::opengl) {
            rocket::log("failed to parse RLSL Shader: critical shader code missing", "shader_i", "rlsl_parser", "error");
            return {};
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

        rlsl_shader.name = get_property("Name");
        rlsl_shader.version = get_property("Version");

        if (rlsl_shader.version.empty() || rlsl_shader.name.empty()) {
            rocket::log("issue while parsing RLSL Shader: shader metadata incomplete", "shader_i", "rlsl_parser", "warn");
        }

        int rlsl_major = std::stoi(split(rlsl_shader.version, '.')[0]);
        int rlsl_minor = std::stoi(split(rlsl_shader.version, '.')[1]);
        if (rlsl_major != ROCKETGE__FEATURE_MAX_RLSL_VERSION_MAJOR 
                || rlsl_minor != ROCKETGE__FEATURE_MAX_RLSL_VERSION_MINOR) {
            rocket::log("This version of RLSL (" + std::to_string(rlsl_major) + "." + std::to_string(rlsl_minor) + " > " + std::to_string(ROCKETGE__FEATURE_MAX_RLSL_VERSION_MAJOR) + "." + std::to_string(ROCKETGE__FEATURE_MAX_RLSL_VERSION_MINOR) + ") is not supported", "shader_i", "rlsl_parser", "error");
            return {};
        }

        int major{}, minor{};
        float glver{};
        auto supported_backends = get_property_ls("API.SupportedAPIs");
        constexpr auto vec_exists = [](auto vec, const auto &elm) -> bool {
            if (std::find(vec.begin(), vec.end(), elm) == vec.end()) {
                return false;
            }

            return true;
        };

        struct {
            int gl_major = 0, gl_minor = 0;
            int gles_major = 0, gles_minor = 0;
            int vk_major = 0, vk_minor = 0;

            bool gl = false;
            bool vk = false;
            bool gles = false;
        } api;

        if (backend == renderer_backend_t::opengl) {
#ifdef ROCKETGE__Platform_Android
            if (vec_exists(supported_backends, "GLES")) {
                const char *ver = reinterpret_cast<const char *>(glGetString(GL_VERSION));
                if (ver != nullptr)
                    sscanf(ver, "OpenGL ES %d.%d", &api.gles_major, &api.gles_minor);
                api.gles = true;

                rlsl_shader.vcode = "precision highp float;" "\n" + rlsl_shader.vcode;
                rlsl_shader.vcode = "#version " + to_hash_version_gl(std::to_string(std::stof(get_property("API.GLES.MinimumVersion")))) + " es\n" + rlsl_shader.vcode;

                rlsl_shader.fcode = "precision highp float;" "\n" + rlsl_shader.fcode;
                rlsl_shader.fcode = "#version " + to_hash_version_gl(std::to_string(std::stof(get_property("API.GLES.MinimumVersion")))) + " es\n" + rlsl_shader.fcode;
            }
#else
            if (vec_exists(supported_backends, "GL")) {
                glGetIntegerv(GL_MAJOR_VERSION, &api.gl_major);
                glGetIntegerv(GL_MINOR_VERSION, &api.gl_minor);
                api.gl = true;

                rlsl_shader.vcode = "#version " + to_hash_version_gl(std::to_string(std::stof(get_property("API.GL.MinimumVersion")))) + "\n" + rlsl_shader.vcode;
                rlsl_shader.fcode = "#version " + to_hash_version_gl(std::to_string(std::stof(get_property("API.GL.MinimumVersion")))) + "\n" + rlsl_shader.fcode;
            }
#endif
        }

        if (backend == renderer_backend_t::vulkan && vec_exists(supported_backends, "VK") && vk_phys_device != nullptr) {
            VkPhysicalDeviceProperties props;
            VkPhysicalDevice dvc = (VkPhysicalDevice) vk_phys_device;
            vkGetPhysicalDeviceProperties(dvc, &props);

            uint32_t ver = props.apiVersion;
            api.vk_major = VK_VERSION_MAJOR(ver);
            api.vk_minor = VK_VERSION_MINOR(ver);
            api.vk = true;
        }

//         glver = major + minor / 10.f;
//         int rlmajor = rlsl_shader.gl_minimumversion;
//         int rlminor = (int)((rlsl_shader.gl_minimumversion - rlmajor) * 10 + 0.5f);
//
//         float vkver{};
//         int vk_major{};
//         int vk_minor{};
//         if (backend == renderer_backend_t::vulkan && vec_exists(supported_backends, "VK")) {
//             if (vk_phys_device != nullptr) {
//                 VkPhysicalDeviceProperties props;
//                 VkPhysicalDevice dvc = (VkPhysicalDevice) vk_phys_device;
//                 vkGetPhysicalDeviceProperties(dvc, &props);
//
//                 uint32_t ver = props.apiVersion;
//                 vk_major = VK_VERSION_MAJOR(ver);
//                 vk_minor = VK_VERSION_MINOR(ver);
//                 vkver = vk_major + (vk_minor / 10.f);
//             }
//         }

        rlsl_parsed_result_t res;

        // if (backend == renderer_backend_t::opengl && vec_exists(supported_backends, "GL")) {
        //     if (rlsl_shader.gles_minimumversion > 0.0) {
        //         if (glver < rlsl_shader.gles_minimumversion) {
        //             PS_ERROR("Invalid GL v")
        //         }
        //     } else if (glver < rlsl_shader.gl_minimumversion) {
        //         rocket::log("issue while parsing RLSL Shader: Loaded OpenGL Version lower than Minimum OpenGL Version (" + std::to_string(major) + "." + std::to_string(minor) + " < " + std::to_string(rlmajor) + "." + std::to_string(rlminor) + + ")", "shader_i", "rlsl_parser", "warn");
        //     }
        // } else if (backend == renderer_backend_t::vulkan) {
        //     if (vkver > 0.f && vkver < rlsl_shader.vk_minimumversion) {
        //         rocket::log("issue while parsing RLSL Shader: Loaded Vulkan Version lower than Minimum Vulkan Version (" + std::to_string(vk_major) + "." + std::to_string(vk_minor) + ")", "shader_i", "rlsl_parser", "warn");
        //     }
        // } else if (backend == renderer_backend_t::null) {
        // } else {
        // }

        res.gl_vert_code = rlsl_shader.vcode;
        res.gl_frag_code = rlsl_shader.fcode;
        res.vk_vert_data = std::move(rlsl_shader.vdata);
        res.vk_frag_data = std::move(rlsl_shader.fdata);
        res.name = rlsl_shader.name;
        res.rlsl_version = rlsl_shader.version;
        res.properties = std::move(state.properties);
        res.properties_list = std::move(state.properties_list);
        return res;
    }
}
