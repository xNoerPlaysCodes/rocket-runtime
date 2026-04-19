#pragma once

#include <string>
#include <vector>
#include <rocket/renderer_helpers.hpp>
#include <filesystem>

namespace rocket {
    struct rlsl_parsed_result_t {
        std::string gl_vert_code;
        std::string gl_frag_code;
        std::vector<uint8_t> vk_vert_data;
        std::vector<uint8_t> vk_frag_data;
        std::string name;
        std::string rlsl_version;

        std::unordered_map<std::string, std::string> properties;
        std::unordered_map<std::string, std::vector<std::string>> properties_list;
    };

    rlsl_parsed_result_t rlsl_parse(std::vector<std::string> lines, std::filesystem::path working_dir, rocket::renderer_backend_t backend, void *vk_phys_device);
}
