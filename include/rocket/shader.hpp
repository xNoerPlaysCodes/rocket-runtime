#ifndef ROCKETGE__SHADERTOOL_HPP
#define ROCKETGE__SHADERTOOL_HPP

#include "types.hpp"
#include <GLFW/glfw3.h>
#include <string>
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
        std::string vcode;
        std::string fcode;

        std::string name = "Unknown";
        std::string rlsl_version = "NOT_COMPILED_BY_RLSL";

        GLuint vao, vbo;
        
        friend class renderer_2d;
        friend class renderer_3d;
    private:
        void shader_init();
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
        shader_t(shader_type type, std::string vcode, std::string fcode, std::string name = "UnknownShader");
        /// @brief Loads a shader from a file (.rlsl)
        shader_t(shader_type type, std::string rlsl_shader_path);
        static shader_t rectangle(rgba_color fill_color);
    public:
        ~shader_t();
    };
}

#endif // ROCKETGE__SHADERTOOL_HPP
