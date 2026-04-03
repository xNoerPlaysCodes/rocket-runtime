#pragma once

#include <string>
#include <vector>
#include <rocket/renderer_helpers.hpp>
#include <filesystem>

namespace rocket {
    void rlsl_parse(std::vector<std::string> lines, std::filesystem::path shader_workingdir, rocket::renderer_backend_t backend, void *vk_phys_device);
}
