#include "rocket/shader.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#ifdef ROCKETGE__Platform_Windows
#include <windows.h>
#endif

#define ROCKETGE__RUNTIME_SKIP_HEADER_INCLUSION
#include <rocket/runtime.hpp>
#include <shader.hpp>
#include <util.hpp>

#include "internal_types.hpp"

namespace rocket {
    namespace {
        enum class shader_stage_t {
            vertex,
            fragment,
        };

        std::string trim_copy(const std::string &value) {
            const auto begin = value.find_first_not_of(" \t\r\n");
            if (begin == std::string::npos) {
                return "";
            }
            const auto end = value.find_last_not_of(" \t\r\n");
            return value.substr(begin, end - begin + 1);
        }

        std::string sanitize_temp_name(std::string_view value) {
            std::string out;
            out.reserve(value.size());
            for (char ch : value) {
                if ((ch >= 'a' && ch <= 'z') ||
                    (ch >= 'A' && ch <= 'Z') ||
                    (ch >= '0' && ch <= '9')) {
                    out.push_back(ch);
                } else {
                    out.push_back('_');
                }
            }
            if (out.empty()) {
                return "shader";
            }
            return out;
        }

        std::filesystem::path glslc_path() {
#ifdef ROCKETGE__Platform_Windows
            if (const char *vulkan_sdk = std::getenv("VULKAN_SDK")) {
                const auto candidate = std::filesystem::path(vulkan_sdk) / "Bin" / "glslc.exe";
                if (std::filesystem::exists(candidate)) {
                    return candidate;
                }
            }
            return std::filesystem::path("glslc.exe");
#else
            return std::filesystem::path("glslc");
#endif
        }

#ifdef ROCKETGE__Platform_Windows
        struct process_result_t {
            DWORD exit_code = static_cast<DWORD>(-1);
            std::string output;
            bool launched = false;
        };

        std::wstring widen(std::string_view value) {
            if (value.empty()) {
                return {};
            }

            const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
            if (size <= 0) {
                return {};
            }

            std::wstring wide(static_cast<std::size_t>(size), L'\0');
            MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), wide.data(), size);
            return wide;
        }

        process_result_t run_process_capture(
            const std::filesystem::path &exe,
            const std::vector<std::wstring> &args
        ) {
            process_result_t result {};

            SECURITY_ATTRIBUTES security_attributes {};
            security_attributes.nLength = sizeof(security_attributes);
            security_attributes.bInheritHandle = TRUE;

            HANDLE read_pipe = nullptr;
            HANDLE write_pipe = nullptr;
            if (!CreatePipe(&read_pipe, &write_pipe, &security_attributes, 0)) {
                result.output = "CreatePipe failed with " + std::to_string(GetLastError());
                return result;
            }
            SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);

            const auto quote_arg = [](const std::wstring &arg) {
                if (arg.find_first_of(L" \t\"") == std::wstring::npos) {
                    return arg;
                }

                std::wstring quoted = L"\"";
                for (const wchar_t ch : arg) {
                    if (ch == L'"') {
                        quoted += L'\\';
                    }
                    quoted += ch;
                }
                quoted += L"\"";
                return quoted;
            };

            const std::wstring exe_w = exe.wstring();
            std::wstring command_line = quote_arg(exe_w);
            for (const auto &arg : args) {
                command_line += L" ";
                command_line += quote_arg(arg);
            }
            std::vector<wchar_t> command_buffer(command_line.begin(), command_line.end());
            command_buffer.push_back(L'\0');

            STARTUPINFOW startup_info {};
            startup_info.cb = sizeof(startup_info);
            startup_info.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
            startup_info.wShowWindow = SW_HIDE;
            startup_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            startup_info.hStdOutput = write_pipe;
            startup_info.hStdError = write_pipe;

            PROCESS_INFORMATION process_info {};
            const BOOL created = CreateProcessW(
                exe_w.c_str(),
                command_buffer.data(),
                nullptr,
                nullptr,
                TRUE,
                CREATE_NO_WINDOW,
                nullptr,
                nullptr,
                &startup_info,
                &process_info
            );
            CloseHandle(write_pipe);
            result.launched = created == TRUE;

            if (!created) {
                result.output = "CreateProcessW failed with " + std::to_string(GetLastError());
                CloseHandle(read_pipe);
                return result;
            }

            WaitForSingleObject(process_info.hProcess, INFINITE);

            char buffer[4096];
            DWORD bytes_read = 0;
            while (ReadFile(read_pipe, buffer, sizeof(buffer), &bytes_read, nullptr) && bytes_read != 0) {
                result.output.append(buffer, buffer + bytes_read);
            }

            GetExitCodeProcess(process_info.hProcess, &result.exit_code);
            CloseHandle(process_info.hThread);
            CloseHandle(process_info.hProcess);
            CloseHandle(read_pipe);
            return result;
        }
#endif

        std::vector<uint32_t> compile_glsl_to_spirv(
            const std::string &source,
            std::string_view stage,
            std::string_view hint
        ) {
            const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
            const auto safe_hint = sanitize_temp_name(hint);
            const auto debug_dir = std::filesystem::current_path() / "build" / "vk_shader_debug";
            std::error_code ignore_ec;
            std::filesystem::create_directories(debug_dir, ignore_ec);
            const auto src_path = debug_dir / ("rocket_vk_shader_" + safe_hint + "_" + std::to_string(nonce) + "." + std::string(stage));
            const auto spv_path = debug_dir / ("rocket_vk_shader_" + safe_hint + "_" + std::to_string(nonce) + ".spv");
            const auto log_path = debug_dir / ("rocket_vk_shader_" + safe_hint + "_" + std::to_string(nonce) + ".log");

            {
                std::ofstream src_file(src_path, std::ios::binary);
                if (!src_file.is_open()) {
                    rocket::log("failed to open temporary shader source file", "vulkan_shader_t", "compile_glsl_to_spirv", "error");
                    return {};
                }
                src_file << source;
            }

            const auto exe = glslc_path();
            std::string command_text;
            int rc = -1;
            std::string compiler_output;
#ifdef ROCKETGE__Platform_Windows
            const auto result = run_process_capture(
                exe,
                {
                    widen("--target-env=vulkan1.0"),
                    widen(std::string("-fshader-stage=") + std::string(stage)),
                    widen("-o"),
                    spv_path.wstring(),
                    src_path.wstring(),
                }
            );
            command_text =
                "\"" + exe.string() + "\"" +
                std::string(" --target-env=vulkan1.0 -fshader-stage=") + std::string(stage) +
                " -o \"" + spv_path.string() + "\" \"" + src_path.string() + "\"";
            rc = static_cast<int>(result.exit_code);
            compiler_output = result.output;
#else
            std::ostringstream command;
            command
                << '"' << exe.string() << '"'
                << " --target-env=vulkan1.0"
                << " -fshader-stage=" << stage
                << " -o " << '"' << spv_path.string() << '"'
                << ' ' << '"' << src_path.string() << '"';
            command_text = command.str();
            rc = std::system(command_text.c_str());
#endif
            if (rc != 0) {
                std::filesystem::remove(spv_path, ignore_ec);
                {
                    std::ofstream log_file(log_path, std::ios::binary);
                    log_file << compiler_output;
                }
                rocket::log(
                    "glslc failed while compiling '" + std::string(hint) + "' (" + std::string(stage) + "')\n"
                        + "command: " + command_text + "\n"
                        + "source: " + src_path.string() + "\n"
                        + "stderr/stdout:\n" + compiler_output,
                    "vulkan_shader_t",
                    "compile_glsl_to_spirv",
                    "error"
                );
                std::fputs(("glslc failed:\n" + command_text + "\n" + compiler_output + "\n").c_str(), stderr);
                return {};
            }
            std::filesystem::remove(src_path, ignore_ec);
            std::filesystem::remove(log_path, ignore_ec);

            std::ifstream spv_file(spv_path, std::ios::binary | std::ios::ate);
            if (!spv_file.is_open()) {
                std::filesystem::remove(spv_path, ignore_ec);
                rocket::log("failed to open generated SPIR-V file", "vulkan_shader_t", "compile_glsl_to_spirv", "error");
                return {};
            }

            const auto end_pos = spv_file.tellg();
            if (end_pos <= 0 || (static_cast<std::size_t>(end_pos) % sizeof(uint32_t)) != 0) {
                std::filesystem::remove(spv_path, ignore_ec);
                rocket::log("generated SPIR-V had an invalid size", "vulkan_shader_t", "compile_glsl_to_spirv", "error");
                return {};
            }

            std::vector<uint32_t> spirv(static_cast<std::size_t>(end_pos) / sizeof(uint32_t));
            spv_file.seekg(0);
            spv_file.read(reinterpret_cast<char*>(spirv.data()), static_cast<std::streamsize>(end_pos));
            spv_file.close();
            std::filesystem::remove(spv_path, ignore_ec);
            return spirv;
        }

        std::vector<uint32_t> bytes_to_words(const std::vector<uint8_t> &bytes) {
            if (bytes.empty() || (bytes.size() % sizeof(uint32_t)) != 0) {
                return {};
            }

            std::vector<uint32_t> out(bytes.size() / sizeof(uint32_t));
            std::memcpy(out.data(), bytes.data(), bytes.size());
            return out;
        }

        bool parse_layout_location(const std::string &line, std::string_view storage, std::string &name, int &location) {
            const std::string trimmed = trim_copy(line);
            if (!trimmed.starts_with("layout(") || trimmed.find(std::string(storage)) == std::string::npos) {
                return false;
            }

            const auto location_pos = trimmed.find("location");
            if (location_pos == std::string::npos) {
                return false;
            }
            const auto equals_pos = trimmed.find('=', location_pos);
            const auto close_pos = trimmed.find(')', equals_pos);
            if (equals_pos == std::string::npos || close_pos == std::string::npos) {
                return false;
            }

            try {
                location = std::stoi(trim_copy(trimmed.substr(equals_pos + 1, close_pos - equals_pos - 1)));
            } catch (...) {
                return false;
            }

            const auto storage_pos = trimmed.find(std::string(storage), close_pos);
            if (storage_pos == std::string::npos) {
                return false;
            }

            std::string remainder = trim_copy(trimmed.substr(storage_pos + storage.size()));
            if (!remainder.empty() && remainder.front() == ' ') {
                remainder.erase(remainder.begin());
            }

            std::stringstream ss(remainder);
            std::string token;
            std::vector<std::string> tokens;
            while (ss >> token) {
                tokens.push_back(token);
            }
            if (tokens.size() < 2) {
                return false;
            }

            name = tokens.back();
            if (!name.empty() && name.back() == ';') {
                name.pop_back();
            }
            return !name.empty();
        }

        std::optional<std::string> translate_interface_line(
            const std::string &line,
            std::string_view storage,
            int location
        ) {
            const std::string trimmed = trim_copy(line);
            if (trimmed.starts_with("layout(") || !trimmed.starts_with(std::string(storage) + " ")) {
                return std::nullopt;
            }

            const auto indent_end = line.find_first_not_of(" \t");
            const std::string indent = indent_end == std::string::npos ? "" : line.substr(0, indent_end);
            return indent + "layout(location = " + std::to_string(location) + ") " + trimmed;
        }

        std::string translate_glsl_for_vulkan(
            const std::string &source,
            shader_stage_t stage,
            std::unordered_map<std::string, int> *shared_locations
        ) {
            std::stringstream ss(source);
            std::string line;
            std::string out;
            bool version_written = false;
            int next_input_location = 0;
            int next_output_location = 0;

            while (std::getline(ss, line)) {
                const std::string trimmed = trim_copy(line);
                if (trimmed.starts_with("#version")) {
                    if (!version_written) {
                        out += "#version 450\n";
                        version_written = true;
                    }
                    continue;
                }

                if (trimmed.starts_with("precision ")) {
                    continue;
                }

                std::string explicit_name;
                int explicit_location = -1;
                if (stage == shader_stage_t::vertex &&
                    parse_layout_location(trimmed, "out", explicit_name, explicit_location) &&
                    shared_locations != nullptr) {
                    (*shared_locations)[explicit_name] = explicit_location;
                    next_output_location = std::max(next_output_location, explicit_location + 1);
                } else if (stage == shader_stage_t::fragment &&
                           parse_layout_location(trimmed, "in", explicit_name, explicit_location)) {
                    next_input_location = std::max(next_input_location, explicit_location + 1);
                }

                if (stage == shader_stage_t::vertex) {
                    if (auto translated = translate_interface_line(line, "in", next_input_location)) {
                        out += *translated + "\n";
                        ++next_input_location;
                        continue;
                    }
                    if (auto translated = translate_interface_line(line, "out", next_output_location)) {
                        const std::string translated_trimmed = trim_copy(*translated);
                        std::stringstream decl_stream(translated_trimmed.substr(translated_trimmed.find("out ") + 4));
                        std::string type_name;
                        std::string var_name;
                        decl_stream >> type_name >> var_name;
                        if (!var_name.empty() && var_name.back() == ';') {
                            var_name.pop_back();
                        }
                        if (shared_locations != nullptr && !var_name.empty()) {
                            (*shared_locations)[var_name] = next_output_location;
                        }
                        out += *translated + "\n";
                        ++next_output_location;
                        continue;
                    }
                } else {
                    if (!trimmed.starts_with("layout(") && trimmed.starts_with("in ")) {
                        std::stringstream decl_stream(trimmed.substr(3));
                        std::string type_name;
                        std::string var_name;
                        decl_stream >> type_name >> var_name;
                        if (!var_name.empty() && var_name.back() == ';') {
                            var_name.pop_back();
                        }
                        int location = next_input_location;
                        if (shared_locations != nullptr) {
                            if (auto it = shared_locations->find(var_name); it != shared_locations->end()) {
                                location = it->second;
                            }
                        }
                        if (auto translated = translate_interface_line(line, "in", location)) {
                            out += *translated + "\n";
                            next_input_location = std::max(next_input_location, location + 1);
                            continue;
                        }
                    }
                    if (auto translated = translate_interface_line(line, "out", next_output_location)) {
                        out += *translated + "\n";
                        ++next_output_location;
                        continue;
                    }
                }

                out += line + "\n";
            }

            if (!version_written) {
                out = "#version 450\n" + out;
            }
            return out;
        }

        api_object_t create_vulkan_shader_object(
            vulkan_renderer_2d &renderer,
            const std::string &name,
            const std::string &vertex_source,
            const std::string &fragment_source,
            const std::vector<uint8_t> *vertex_blob = nullptr,
            const std::vector<uint8_t> *fragment_blob = nullptr
        ) {
            vk_shader_t shader {};
            shader.name = name;
            shader.vertex_source = vertex_source;
            shader.fragment_source = fragment_source;

            if (vertex_blob != nullptr && fragment_blob != nullptr &&
                !vertex_blob->empty() && !fragment_blob->empty()) {
                shader.vertex_spirv = bytes_to_words(*vertex_blob);
                shader.fragment_spirv = bytes_to_words(*fragment_blob);
            }

            if (shader.vertex_spirv.empty() || shader.fragment_spirv.empty()) {
                std::unordered_map<std::string, int> varying_locations;
                const std::string vk_vertex_source =
                    translate_glsl_for_vulkan(vertex_source, shader_stage_t::vertex, &varying_locations);
                const std::string vk_fragment_source =
                    translate_glsl_for_vulkan(fragment_source, shader_stage_t::fragment, &varying_locations);

                shader.vertex_spirv = compile_glsl_to_spirv(vk_vertex_source, "vert", name + "_vert");
                shader.fragment_spirv = compile_glsl_to_spirv(vk_fragment_source, "frag", name + "_frag");
            }

            if (shader.vertex_spirv.empty() || shader.fragment_spirv.empty()) {
                rocket::log("failed to compile Vulkan shader '" + name + "'", "vulkan_shader_t", "shader_init", "error");
                return 0;
            }

            const api_object_t handle = renderer.allocate_object_handle();
            vk_object_t object {};
            object.type = vk_object_type_t::shader;
            object.value = std::move(shader);
            renderer.get_backend_impl()->objects[handle] = std::move(object);
            return handle;
        }
    }

    void vulkan_shader_t::shader_init() {
        auto *renderer = dynamic_cast<vulkan_renderer_2d*>(util::get_global_renderer_2d());
        if (renderer == nullptr) {
            rocket::log("global renderer was not Vulkan while initializing a Vulkan shader", "vulkan_shader_t", "shader_init", "error");
            this->hdl = 0;
            return;
        }

        this->hdl = create_vulkan_shader_object(*renderer, this->name, this->vcode, this->fcode);
    }

    vulkan_shader_t::vulkan_shader_t(shader_type type, std::string vcode, std::string fcode, std::string name) {
        this->type = type;
        this->vcode = std::move(vcode);
        this->fcode = std::move(fcode);
        this->name = std::move(name);

        this->shader_init();
    }

    void vulkan_shader_t::parse(const std::vector<std::string> &lines, std::filesystem::path shader_workingdir) {
        rlsl_parsed_result_t res = rocket::rlsl_parse(lines, shader_workingdir, renderer_backend_t::vulkan, nullptr);

        this->vcode = res.gl_vert_code;
        this->fcode = res.gl_frag_code;
        this->name = res.name;
        this->rlsl_version = res.rlsl_version;

        auto *renderer = dynamic_cast<vulkan_renderer_2d*>(util::get_global_renderer_2d());
        if (renderer == nullptr) {
            rocket::log("global renderer was not Vulkan while loading an RLSL Vulkan shader", "vulkan_shader_t", "parse", "error");
            this->hdl = 0;
            return;
        }

        this->hdl = create_vulkan_shader_object(
            *renderer,
            this->name,
            this->vcode,
            this->fcode,
            &res.vk_vert_data,
            &res.vk_frag_data
        );

        rocket::log("Loaded RLSL Shader: " + res.name, "vulkan_shader_t", "constructor", "info");
    }

    vulkan_shader_t::vulkan_shader_t(shader_type type, std::filesystem::path rlsl_shader_path) {
        this->type = type;
        std::ifstream istream(rlsl_shader_path);
        const std::filesystem::path shader_workingdir = rlsl_shader_path.parent_path();
        if (!istream.is_open()) {
            rocket::log("failed to open file path: " + rlsl_shader_path.string(), "vulkan_shader_t", "constructor", "error");
            return;
        }

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(istream, line)) {
            lines.push_back(line);
        }

        this->parse(lines, shader_workingdir);
    }

    vulkan_shader_t::vulkan_shader_t() = default;

    vulkan_shader_t::vulkan_shader_t(shader_type type, const std::string &rlsl) {
        this->type = type;

        std::stringstream ss(rlsl);
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(ss, line)) {
            lines.push_back(line);
        }

        this->parse(lines, std::filesystem::current_path());
    }

    void vulkan_shader_t::set_parameter(std::string name, float value) {
        (void) name;
        (void) value;
    }

    void vulkan_shader_t::set_parameter(std::string name, int value) {
        (void) name;
        (void) value;
    }

    void vulkan_shader_t::set_parameter(std::string name, vec2f_t value) {
        (void) name;
        (void) value;
    }

    void vulkan_shader_t::set_parameter(std::string name, vec3f_t value) {
        (void) name;
        (void) value;
    }

    void vulkan_shader_t::set_parameter(std::string name, vec4f_t value) {
        (void) name;
        (void) value;
    }

    void vulkan_shader_t::set_parameter(std::string name, mat4 value) {
        (void) name;
        (void) value;
    }

    void vulkan_shader_t::set_parameter_raw(std::string name, uint32_t type, const void *data, int count) {
        (void) name;
        (void) type;
        (void) data;
        (void) count;
    }

    bool vulkan_shader_t::operator==(const shader_i &other) const {
        const auto *vk_other = dynamic_cast<const vulkan_shader_t*>(&other);
        if (vk_other == nullptr) {
            return false;
        }
        return this->hdl == vk_other->hdl;
    }

    vulkan_shader_t::~vulkan_shader_t() = default;
}
