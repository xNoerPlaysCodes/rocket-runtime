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
    static std::unordered_map<shader_id_t, shader_t> shader_map;

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

    rgl::shader_program_t get_shader(shader_id_t shid) {
        if (rocket::globals::g_main_thread_id_set) 
            r_assert(globals::g_main_thread_id == std::this_thread::get_id() && "rocket::get_shader called on worker thread");
        if (shader_map.find(shid) != shader_map.end())
            return shader_map[shid].glprogram;

#define SHADER_DISPATCH_ENTRY(name)                                      \
        if (shid == shader_id_t::name) {                                 \
            shader_map[shid] = shader_t::load_from_rlsl_source(          \
                shader_type::vert_frag,                                  \
                rocket_resource::CONCAT(shader_, CONCAT(name, _rlsl)));  \
            return shader_map[shid].glprogram;                           \
        }
#include <resources/autogen_shader_dispatch.h>
#undef SHADER_DISPATCH_ENTRY

        rocket::log("invalid shader_id given", "rocket", "get_shader", "fatal");
        rocket::exit(1);
        return rGL_SHADER_INVALID;
    }

    void shader_provider_compile_all() {
        using namespace rocket_resource;
        for (shader_id_t id : {SHADER_ID_LIST}) {
            get_shader(id);
        }
    }

    void shader_provider_reset() {
        shader_map.clear();
    }
}
