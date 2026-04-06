#include "rocket/shader.hpp"
#include <string>
#include <fstream>
#define ROCKETGE__RUNTIME_SKIP_HEADER_INCLUSION
#include <rocket/runtime.hpp>
#include <shader.hpp>
#include <util.hpp>

namespace rocket {
    void vulkan_shader_t::shader_init() {
    }

    vulkan_shader_t::vulkan_shader_t(shader_type type, std::string vcode, std::string fcode, std::string name) {
        this->type = type;
        this->vcode = vcode;
        this->fcode = fcode;
        this->name = name;

        this->shader_init();
    }

    void vulkan_shader_t::parse(const std::vector<std::string> &lines, std::filesystem::path shader_workingdir) {
        rlsl_parsed_result_t res = rocket::rlsl_parse(lines, shader_workingdir, util::get_renderer_backend(util::get_global_renderer_2d()), nullptr);

        this->shader_init();

        rocket::log("Loaded RLSL Shader: " + res.name, "vulkan_shader_t", "constructor", "info");
    }

    vulkan_shader_t::vulkan_shader_t(shader_type type, std::filesystem::path rlsl_shader_path) {
        this->type = type;
        std::ifstream istream(rlsl_shader_path);
        std::filesystem::path shader_workingdir = rlsl_shader_path.parent_path();
        if (!istream.is_open()) {
            rocket::log("failed to open file path: " + rlsl_shader_path.string(), "vulkan_shader_t::vulkan_shader_t(shader_type, std", "string)", "error");
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

    vulkan_shader_t::vulkan_shader_t() {}

    vulkan_shader_t::vulkan_shader_t(shader_type type, const std::string &rlsl) {
    }

    void vulkan_shader_t::set_parameter(std::string name, float value) {
    }

    void vulkan_shader_t::set_parameter(std::string name, int value) {
    }

    void vulkan_shader_t::set_parameter(std::string name, vec2f_t value) {
    }

    void vulkan_shader_t::set_parameter(std::string name, vec3f_t value) {
    }

    void vulkan_shader_t::set_parameter(std::string name, vec4f_t value) {
    }

    void vulkan_shader_t::set_parameter(std::string name, mat4 value) {
    }

    void vulkan_shader_t::set_parameter_raw(std::string name, uint32_t type, const void* data, int count) {
    }

    bool vulkan_shader_t::operator==(const shader_i &other) const {
        return false;
    }

    vulkan_shader_t::~vulkan_shader_t() {
    }
}
