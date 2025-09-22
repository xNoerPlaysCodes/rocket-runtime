#ifndef ROCKETGE_SUBSPLUGIN__API_HPP
#define ROCKETGE_SUBSPLUGIN__API_HPP

#include "rocket/renderer.hpp"
#include <chrono>
#include <functional>
#include <string>

struct plugin_capabilities_t {
    bool is_lazy_loadable = false;
    
    // Exposure::Rendering
    bool needs_expo_renderer2d = false;
    bool needs_frame_events = false;
};

namespace rapi {
    struct api_t {
        std::string api_version = "0.0.0";
        int api_iteration = -1;

        // Logger
        std::function<void(std::string error, int error_id, std::string error_source, std::string level)> log_error;
        std::function<void(std::string error, std::string class_file_library_source, std::string function_source, std::string level)> log;

        /// @brief Get User-Exposed Renderer 2D
        /// @note Return value and the function itself may be nullptr
        /// @note Only callable AFTER on_load()
        /// @note Only defined if plugin_capabilities_t::needs_expo_renderer2d is true
        std::function<rocket::renderer_2d*()> get_renderer2d;
    };
}

#endif // ROCKETGE_SUBSPLUGIN__API_HPP
