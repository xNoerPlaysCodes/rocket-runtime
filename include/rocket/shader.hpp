#ifndef ROCKETGE__SHADERTOOL_HPP
#define ROCKETGE__SHADERTOOL_HPP

#include "types.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace rocket {
    enum class shader_id_t;
    struct vk_shader_t;
    uint32_t gl_get_shader(rocket::shader_id_t shid);
    vk_shader_t vk_get_shader(rocket::shader_id_t shid);
}

namespace rgl {
    typedef unsigned int shader_program_t;
    rgl::shader_program_t gl_get_shader(rocket::shader_id_t shid);
}

namespace rocket {
    enum class shader_type {
        vert_frag,
    };

    class shader_i {
    protected:
        shader_type type;
        std::string name = "NON_RLSL_SHADER";
        std::string rlsl_version = "NOT_COMPILED_BY_RLSL";

        std::string vcode;
        std::string fcode;
        
        friend class renderer_2d_i;
        friend class opengl_renderer_2d;
        friend class vulkan_renderer_2d;
        friend class renderer_3d;
        friend void shader_provider_reset();
    protected:
        virtual void shader_init() = 0;
        virtual void parse(const std::vector<std::string> &lines, std::filesystem::path shader_workingdir) = 0;
    public:
        virtual void set_parameter(std::string name, float value) = 0;
        virtual void set_parameter(std::string name, int value) = 0;
        virtual void set_parameter(std::string name, vec2f_t value) = 0;
        virtual void set_parameter(std::string name, vec3f_t value) = 0;
        virtual void set_parameter(std::string name, vec4f_t value) = 0;
        virtual void set_parameter(std::string name, mat4 value) = 0;
        virtual void set_parameter_raw(std::string name, unsigned int type, const void* data, int count) = 0;
    public:
        virtual bool operator==(const shader_i &other) const = 0;
    public:
        virtual ~shader_i() = default;
    };

    class opengl_shader_t : public shader_i {
    private:
        uint32_t glshaderv = 0xDEADBEEFU;
        uint32_t glshaderf = 0xDEADBEEFU;
        uint32_t glprogram = 0xDEADBEEFU;

        uint32_t vao = 0xDEADBEEFU;
        uint32_t vbo = 0xDEADBEEFU;

        friend rgl::shader_program_t get_shader(shader_id_t shid);
        friend uint32_t rocket::gl_get_shader(shader_id_t shid);
        friend class opengl_renderer_2d;
    private:
        void shader_init() override;
        void parse(const std::vector<std::string> &lines, std::filesystem::path shader_workingdir) override;
    public:
        void set_parameter(std::string name, float value) override;
        void set_parameter(std::string name, int value) override;
        void set_parameter(std::string name, vec2f_t value) override;
        void set_parameter(std::string name, vec3f_t value) override;
        void set_parameter(std::string name, vec4f_t value) override;
        void set_parameter(std::string name, mat4 value) override;
        void set_parameter_raw(std::string name, unsigned int type, const void* data, int count) override;
    public:
        bool operator==(const shader_i &other) const override;
    public:
        opengl_shader_t(shader_type type, std::string vcode, std::string fcode, std::string name = "NON_RLSL_SHADER");
        /// @brief Loads a shader from a file (.rlsl)
        opengl_shader_t(shader_type type, std::filesystem::path rlsl_shader_path);
        opengl_shader_t(shader_type type, const std::string &rlsl_source);
        opengl_shader_t();
    public:
        ~opengl_shader_t() override;
    };

    struct vulkan_shader_t : public shader_i {
    private:
        api_object_t hdl;
        friend class vulkan_renderer_2d;
    private:
        void shader_init() override;
        void parse(const std::vector<std::string> &lines, std::filesystem::path shader_workingdir) override;

        friend vk_shader_t vk_get_shader(shader_id_t shader);
        friend vk_shader_t rocket::vk_get_shader(shader_id_t shid);
    public:
        void set_parameter(std::string name, float value) override;
        void set_parameter(std::string name, int value) override;
        void set_parameter(std::string name, vec2f_t value) override;
        void set_parameter(std::string name, vec3f_t value) override;
        void set_parameter(std::string name, vec4f_t value) override;
        void set_parameter(std::string name, mat4 value) override;
        void set_parameter_raw(std::string name, unsigned int type, const void* data, int count) override;
    public:
        bool operator==(const shader_i &other) const override;
    public:
        vulkan_shader_t(shader_type type, std::string vcode, std::string fcode, std::string name = "NON_RLSL_SHADER");
        /// @brief Loads a shader from a file (.rlsl)
        vulkan_shader_t(shader_type type, std::filesystem::path rlsl_shader_path);
        vulkan_shader_t(shader_type type, const std::string &rlsl_source);
        vulkan_shader_t();
    public:
        ~vulkan_shader_t() override;
    };
}

#endif // ROCKETGE__SHADERTOOL_HPP
