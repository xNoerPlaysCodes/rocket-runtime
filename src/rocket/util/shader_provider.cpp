#include <rocket/runtime.hpp>
#include <rocket/shader.hpp>
#include <shader_provider.hpp>
#include <unordered_map>
#include <resources/autogen_shader_includes.h>
#include "rocket/macros.hpp"
#if defined(ROCKETGE__Platform_Android)
    #include <GLES3/gl32.h>
    #include <EGL/egl.h>
#else
    #include <GL/glew.h>
#endif
#include "intl_macros.hpp"
// NOTE: no autogen_shader_dispatch.h up here

namespace rocket::globals {
    extern std::thread::id g_main_thread_id;
    extern bool g_main_thread_id_set;
}

namespace rocket {
    static std::unordered_map<shader_id_t, opengl_shader_t> gl_shader_map;
    static std::unordered_map<shader_id_t, vulkan_shader_t> vk_shader_map;

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

    rgl::shader_program_t gl_get_shader(shader_id_t shid) {
        r_debug_if (rocket::globals::g_main_thread_id_set)
            r_assert(globals::g_main_thread_id == std::this_thread::get_id() && "rocket::get_shader called on worker thread");
        if (gl_shader_map.find(shid) != gl_shader_map.end())
            return gl_shader_map[shid].glprogram;

#define SHADER_DISPATCH_ENTRY(name)                                      \
        if (shid == shader_id_t::name) {                                 \
            gl_shader_map[shid] = opengl_shader_t(          \
                shader_type::vert_frag,                                  \
                std::string(rocket_resource::CONCAT(shader_, CONCAT(name, _rlsl))));  \
            return gl_shader_map[shid].glprogram;                           \
        }
#include <resources/autogen_shader_dispatch.h>
#undef SHADER_DISPATCH_ENTRY

        rocket::log("invalid shader_id given", "rocket", "get_shader", "fatal");
        rocket::exit(1);
        return rGL_SHADER_INVALID;
    }

    vk_shader_t vk_get_shader(shader_id_t shid) {
        r_debug_if (rocket::globals::g_main_thread_id_set)
            r_assert(globals::g_main_thread_id == std::this_thread::get_id() && "rocket::get_shader called on worker thread");
        if (vk_shader_map.find(shid) != vk_shader_map.end()) {
            auto *ren = util::get_global_renderer_2d();
            auto *vk_ren = dynamic_cast<vulkan_renderer_2d*>(ren);
            r_assert(vk_ren != nullptr && "renderer was not of type vulkan_renderer_2d");
            vk_object_t obj = vk_ren->bk_impl->objects[vk_shader_map[shid].hdl];
            vk_shader_t shader = std::get<vk_shader_t>(obj.value);
            return shader;
        }

#define SHADER_DISPATCH_ENTRY(name)                                      \
        if (shid == shader_id_t::name) {                                 \
            vk_shader_map[shid] = vulkan_shader_t(          \
                shader_type::vert_frag,                                  \
                std::string(rocket_resource::CONCAT(shader_, CONCAT(name, _rlsl))));  \
            auto *ren = util::get_global_renderer_2d(); \
            auto *vk_ren = dynamic_cast<vulkan_renderer_2d*>(ren); \
            r_assert(vk_ren != nullptr && "renderer was not of type vulkan_renderer_2d"); \
            vk_object_t obj = vk_ren->bk_impl->objects[vk_shader_map[shid].hdl]; \
            vk_shader_t shader = std::get<vk_shader_t>(obj.value); \
            return shader; \
        }
#include <resources/autogen_shader_dispatch.h>
#undef SHADER_DISPATCH_ENTRY

        rocket::log("invalid shader_id given", "rocket", "get_shader", "fatal");
        rocket::exit(1);
        r_assert(false && "invalid shader id given");
        return {};
    }

    void shader_provider_compile_all_gl() {
        using namespace rocket_resource;
        for (shader_id_t id : {SHADER_ID_LIST}) {
            gl_get_shader(id);
        }
    }

    void shader_provider_compile_all_vk() {
        using namespace rocket_resource;
        for (shader_id_t id : {SHADER_ID_LIST}) {
            vk_get_shader(id);
        }
    }

    void shader_provider_reset() {
        gl_shader_map.clear();
        vk_shader_map.clear();
    }
}
