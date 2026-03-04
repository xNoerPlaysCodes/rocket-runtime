#ifndef ROCKETGE_SUBSPLUGIN__PLUGIN_HPP
#define ROCKETGE_SUBSPLUGIN__PLUGIN_HPP

#include "rocket/plugin/api/api.hpp"
#include <filesystem>
#include <string>

#include <functional>

#ifdef ROCKETGE__Platform_Windows
#include <windows.h>
typedef HMODULE lib_handle;
#else
#include <dlfcn.h>
typedef void* lib_handle;
#endif

namespace rocket {
    struct plugin_t {
    private:
        std::filesystem::path path;
        lib_handle handle;
        plugin_capabilities_t *cap = nullptr;

        friend std::shared_ptr<plugin_t> load_plugin(std::filesystem::path plugin);
        friend plugin_capabilities_t *handle_init_plugin(lib_handle handle);
        friend void __rallframestart();
        friend void __rallframeend();
    public:
        std::string name = "";
        std::string author = "";
        std::string version = "";
        std::string description = "";

        /// @brief Get a function from the plugin
        /// @note Returns nullptr if not found
        void *get_function(std::string name);
    };

    std::shared_ptr<plugin_t> load_plugin(std::filesystem::path plugin);
}

#endif // ROCKETGE_SUBSPLUGIN__PLUGIN_HPP
