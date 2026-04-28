#pragma once

#include <cstdint>
#include <functional>
#include <glm/matrix.hpp>
#include <sgfx/types.hpp>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <span>

namespace sgfx {
    enum class colorspace_e {
        linear = 0,
        srgb
    };

    struct context_options_t {
        bool multisampling = false;
        bool double_buffer = true;
        bool depth_buffer = true;
        bool debug = true;
        colorspace_e preferred_colorspace = colorspace_e::linear;
    };
}

namespace sgfx::gl {
    enum class obj_type_e {
        context,
        draw_data,
        shader,
        texture,
    };

    struct context_t {
        sgfx::context_options_t options;
    };

    struct draw_data_t {
        unsigned int vao, vbo, ebo;
    };

    struct shader_t {
        unsigned int program;
        operator unsigned int() {
            return this->program;
        }
    };

    struct texture_t {
        unsigned int tx;
        operator unsigned int() {
            return this->tx;
        }
    };

    struct object_t {
        obj_type_e type;
        std::variant<
            std::monostate,
            context_t,
            draw_data_t,
            shader_t,
            texture_t
        > data;
    }; 
}

namespace sgfx::vk {
    enum class obj_type_e {
        context,
    };

    struct object_t {};
    struct context_t {};
}

namespace sgfx {
    using gpu_object_t = uint32_t;
    using unique_object_id_t = uint32_t;
    namespace internal {
        struct compiled_object_t {
            gpu_object_t draw_data;
            gpu_object_t shader;
        };
    }

    struct context_t {
        gpu_object_t id;
        std::unordered_map<gpu_object_t, gl::object_t> gl_objects;
        std::unordered_map<gpu_object_t, vk::object_t> vk_objects;
        std::unordered_map<unique_object_id_t, internal::compiled_object_t> unique_objects;
        glm::mat4 proj;
    };

    enum class backend_e {
        null = 0,
        opengl,
        vulkan
    };

    namespace internal {
        bool is_object_vulkan(const context_t &, gpu_object_t obj);
        bool is_object_opengl(const context_t &, gpu_object_t obj);
    }

    template<typename NativeObject>
    NativeObject object_cast(context_t &ctx, gpu_object_t obj) {
        if constexpr (std::is_same_v<gl::object_t, NativeObject>) {
            if (internal::is_object_opengl(ctx, obj)) {
                return ctx.gl_objects.at(obj);
            }
        } else if constexpr (std::is_same_v<vk::object_t, NativeObject>) {
            if (internal::is_object_vulkan(ctx, obj)) {
                return ctx.vk_objects.at(obj);
            }
        }

        throw std::runtime_error("Invalid function usage: 'NativeObject' is not any of handled [backend]::object_t types");
    }

    void destroy_object(context_t &c, gpu_object_t obj);

    sgfx::context_t create_context(const sgfx::context_options_t &);

    std::vector<std::string> get_context_creation_log_messages(
        backend_e bk,
        std::string driver_string,
        std::string api_version,
        int loaded_extensions,
        std::vector<std::pair<std::string, std::string>> features,
        bool debug,
        std::string gpu_name,
        std::string gpu_vendor
    ) noexcept;

    class shader_t {
    public:
        gpu_object_t obj = 0;
        std::string vertex_src;
        std::string fragment_src;
    public:
        shader_t(std::string v, std::string f) : vertex_src(v), fragment_src(f) {}
        shader_t() {}
        static shader_t from(context_t &ctx, unsigned int gl_program) noexcept;
        bool compile(context_t &ctx, std::string *output_log = nullptr) noexcept;
        void gl_provide_precompiled(context_t &ctx, unsigned int gl_program) noexcept;
    };

    uint32_t allocate_id();

    struct vertex_t {
        vec3f_t pos;
        vec2f_t uv;
    };

    struct draw_data_t {
        std::vector<vertex_t> vertices;
        std::vector<uint32_t> indices;
    };

    struct draw_object_t {
        shader_t shader;
        draw_data_t draw_data;
    };

    struct transform_t {
        vec3f_t position;
        float32_t rotation;
        vec3f_t scale;
    };

    struct texture_t {
        gpu_object_t tx = 0;
        std::span<uint8_t> data;
        vec2i_t size;
        int channels;

        void upload_to_gpu(context_t &ctx) noexcept;
        void bind(const context_t &ctx) const noexcept;
        ~texture_t();
    };

    struct visual_effects_t {
        rgba_color color = rgba_color::black();
    };

    struct draw_instructions_t {
        bool wireframe = false;
    };

    struct shader_parameter_t {
        std::string key;
        std::variant<
            std::monostate,
            int,
            float,
            std::array<float, 2>,
            std::array<float, 3>,
            std::array<float, 4>,
            std::array<std::array<float, 4>, 4>
        > value;
    };

    struct render_stats_t {};

    struct drawcall_t {
        unique_object_id_t id;
        draw_object_t draw_obj;
        transform_t transform;
        visual_effects_t visual_effects;
        draw_instructions_t draw_instructions;
        std::vector<shader_parameter_t> shader_params;
        std::function<void(unsigned int vao)> _post_bind_vao = nullptr;
    };

    class renderer_t {
    public:
        sgfx::fbounding_box viewport;
        std::function<vec2i_t()> framebuffer_size_poller;
        sgfx::context_t *ctx = nullptr;
        std::vector<drawcall_t> drawcalls;
    public:
        void enqueue(
            unique_object_id_t unique_object_id,
            const draw_object_t &object, 
            const transform_t &transform,
            const visual_effects_t &visual_effects,
            const draw_instructions_t &instructions = {},
            const std::vector<shader_parameter_t> &shader_params = {}
        ) noexcept;

        void enqueue(const drawcall_t &dc) noexcept;

        void begin_frame() noexcept;
        void clear(rgba_color color) noexcept;
        void end_frame() noexcept;

        void flush_queue() noexcept;

        render_stats_t get_stats() const noexcept;
    public:
        renderer_t(sgfx::context_t *ctx, std::function<vec2i_t()> framebuffer_size_poller);
    };

    void tx_allocator_init(int max_units);

    /// @brief Allocate Texture Unit
    /// @param dst Unit to write to
    /// @return bool success
    bool tx_alloc(int &dst);
    /// @brief Free Texture Unit
    /// @param dst Which unit
    void tx_free(int &dst);

    void tx_bind(int tx);

    void cleanup_all();
}
