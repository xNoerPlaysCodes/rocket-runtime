#ifndef ROCKETGE__SHADERTOOL_HPP
#define ROCKETGE__SHADERTOOL_HPP

#include "types.hpp"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <rocket/rgl.hpp>
#include <string>

namespace rocket {
    enum class shader_id_t;
}

namespace rgl {
    rgl::shader_program_t get_shader(rocket::shader_id_t shid);
}

namespace rocket {
    enum class shader_type {
        vert_frag,
    };

    class shader_t {
    private:
        GLuint glshaderv = 0;
        GLuint glshaderf = 0;
        GLuint glprogram = 0;
        shader_type type;
        std::string vcode = "";
        std::string fcode = "";

        std::string name = "NON_RLSL_SHADER";
        std::string rlsl_version = "NOT_COMPILED_BY_RLSL";

        GLuint vao{0}, vbo{0};
        
        friend class renderer_2d;
        friend class renderer_3d;
        friend rgl::shader_program_t get_shader(shader_id_t shid);
        friend shader_t load_from_rlsl_source(shader_type type, std::string rlsl);
    private:
        void shader_init();
        void parse(const std::vector<std::string> &lines, std::filesystem::path shader_workingdir);
    public:
        void set_uniform(std::string name, float value);
        void set_uniform(std::string name, int value);
        void set_uniform(std::string name, vec2f_t value);
        void set_uniform(std::string name, vec3f_t value);
        void set_uniform(std::string name, vec4f_t value);
        void set_uniform(std::string name, mat4 value);
        void set_uniform_raw(std::string name, GLenum type, const void* data, GLsizei count);
    public:
        bool operator==(const shader_t &other) const {
            return glprogram == other.glprogram;
        }
    public:
        shader_t(shader_type type, std::string vcode, std::string fcode, std::string name = "NON_RLSL_SHADER");
        /// @brief Loads a shader from a file (.rlsl)
        shader_t(shader_type type, std::string rlsl_shader_path);
        shader_t();
        static shader_t load_from_rlsl_source(shader_type type, std::string rlsl);
        static shader_t rectangle(rgba_color fill_color);
    public:
        ~shader_t();
    };
}

#endif // ROCKETGE__SHADERTOOL_HPP
