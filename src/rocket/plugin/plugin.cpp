#include "rocket/plugin/plugin.hpp"
#include "rocket/macros.hpp"
#include "rocket/plugin/api/api.hpp"
#include "rocket/runtime.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <rocket/threads.hpp>
#include <vector>
#include "util.hpp"

#include <miniz.h>

#error fix windows GetProcAddress returns FARPROC
#ifdef ROCKETGE__Platform_Windows
#include <windows.h>
#define load_lib(name) LoadLibraryA(name)
#define get_func(handle, func) (void*) GetProcAddress(handle, func)
#define close_lib(handle) FreeLibrary(handle)

constexpr std::string dll_extension = ".dll";
#else
#include <dlfcn.h>
#define load_lib(name) dlopen(name, RTLD_LAZY)
#define get_func(handle, func) dlsym(handle, func)
#define close_lib(handle) dlclose(handle)

constexpr std::string dll_extension = ".so";
#endif

constexpr std::string   api_version     = "1.0-dev";
constexpr int           api_iteration   = 1;

#define CALL_FUN_NOARG(p, ret, nm) ((ret (*)())(p->get_function(nm)))();

namespace rocket {
    std::vector<std::shared_ptr<plugin_t>> loaded_plugins;

    void *plugin_t::get_function(std::string name) {
        if (this->handle == nullptr) {
            rocket::log("plugin not loaded", "plugin_t", "get_function", "error");
            return nullptr;
        }

        return get_func(this->handle, name.c_str());
    }

    rapi::api_t *api = nullptr;

    plugin_capabilities_t *handle_init_plugin(lib_handle handle) {
        if (handle == nullptr) {
            return nullptr;
        }

        auto on_load = (plugin_capabilities_t* (*)(rapi::api_t*)) (get_func(handle, "on_load"));
        auto on_unload = (void (*)(rapi::api_t*)) (get_func(handle, "on_unload"));

        if (api == nullptr) {
            api = new rapi::api_t;
            api->api_impl_version = api_version;
            api->log = &rocket::log;
            api->get_plugins = []() -> std::vector<std::shared_ptr<rocket::plugin_t>> {
                return loaded_plugins;
            };
            api->schedule_gl = &thread_t::schedule;
            api->schedule_now = &thread_t::run;
        }

        if (on_load == nullptr || on_unload == nullptr) {
            rocket::log("failed to get on_load or on_unload", "plugin.cpp", "handle_init_plugin", "error");
            return nullptr;
        }

        plugin_capabilities_t *cap = on_load(api);
        if (cap == nullptr) {
            rocket::log("failed to get plugin capabilities", "plugin.cpp", "handle_init_plugin", "error");
            return nullptr;
        }

        if (cap->needs_expo_renderer2d) {
            api->get_renderer2d = &util::get_global_renderer_2d;
        }

        if (cap->needs_frame_events) {
            auto on_framestart = reinterpret_cast<void(*)()>(get_func(handle, "on_framestart"));
            auto on_frameend = reinterpret_cast<void(*)()>(get_func(handle, "on_frameend"));
            if (on_framestart == nullptr || on_frameend == nullptr) {
                rocket::log("failed to get on_framestart or on_frameend while capabilities needs_frame_events is true", "plugin.cpp", "handle_init_plugin", "error");
                return nullptr;
            }
        }

        if (cap->is_lazy_loadable) {
            rocket::log("lazy loadable plugins not supported", "plugin.cpp", "handle_init_plugin", "error");
        }

        return cap;
    }

    void __rallframestart() {
        for (auto &plugin : loaded_plugins) {
            auto on_framestart = reinterpret_cast<void(*)()>(plugin->get_function("on_framestart"));
            auto on_frameend = reinterpret_cast<void(*)()>(plugin->get_function("on_frameend"));

            if (on_framestart == nullptr || on_frameend == nullptr) {
                continue;
            }

            if (plugin->cap->needs_frame_events) {
                on_framestart();
            }
        }
    }

    void __rallframeend() {
        for (auto &plugin : loaded_plugins) {
            auto on_framestart = reinterpret_cast<void(*)()>(plugin->get_function("on_framestart"));
            auto on_frameend = reinterpret_cast<void(*)()>(plugin->get_function("on_frameend"));

            if (on_framestart == nullptr || on_frameend == nullptr) {
                continue;
            }

            if (plugin->cap->needs_frame_events) {
                on_frameend();
            }
        }
    }

    std::shared_ptr<plugin_t> load_plugin(std::filesystem::path plugin) {
        if (util::get_clistate().noplugins) {
            return nullptr;
        }
        if (!std::filesystem::exists(plugin)) {
            rocket::log("plugin does not exist", "plugin.cpp", "load_plugin", "error");
            return nullptr;
        }

        mz_zip_archive zip;
        memset(&zip, 0, sizeof(zip));
        
        if (!mz_zip_reader_init_file(&zip, plugin.string().c_str(), 0)) {
            rocket::log("failed to open plugin", "plugin.cpp", "load_plugin", "error");
            return nullptr;
        }

        int num_files = static_cast<int>(mz_zip_reader_get_num_files(&zip));

        std::shared_ptr<plugin_t> p = std::make_shared<plugin_t>();
        for (int i = 0; i < num_files; ++i) {
            mz_zip_archive_file_stat stat;
            if (!mz_zip_reader_file_stat(&zip, i, &stat)) {
                rocket::log("failed to get file stats", "plugin.cpp", "load_plugin", "error");
                return nullptr;
            }

            std::string filename = stat.m_filename;
            if (filename == "plugin.json") {
                rocket::log("[fixme] read plugin.json", "plugin.cpp", "load_plugin", "info");
            } else if (filename == ("entrypoint" + dll_extension)) {
                size_t lib_size = static_cast<size_t>(stat.m_uncomp_size);
                std::vector<uint8_t> buffer(lib_size);

                if (!mz_zip_reader_extract_to_mem(&zip, i, buffer.data(), lib_size, 0)) {
                    rocket::log("failed to extract library from plugin", "plugin.cpp", "load_plugin", "error");
                    return nullptr;
                }

                // Write temp file
                std::filesystem::path tmp_path = std::filesystem::temp_directory_path() / filename;
                std::ofstream tmp_file(tmp_path, std::ios::binary);
                tmp_file.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
                tmp_file.close();

                lib_handle handle = load_lib(tmp_path.string().c_str());
                if (handle == nullptr) {
                    rocket::log("failed to load extracted library", "plugin.cpp", "load_plugin", "error");
                    return nullptr;
                }

                p->path = plugin;
                p->handle = handle;

                p->cap = handle_init_plugin(handle);
            }
        }

        loaded_plugins.push_back(p);
        return p;
    }
}
