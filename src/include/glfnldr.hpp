#ifndef ROCKETGE__GLFNLDR_HPP
#define ROCKETGE__GLFNLDR_HPP

namespace glfnldr {
    enum class backend_t : int {
        null = 0,
        glew,
        libepoxy,
        glfw // Unsupported FIXME
    };

    /// @brief (Try) to load OpenGL functions use a function loader backend
    bool init(backend_t backend);
}

#endif//ROCKETGE__GLFNLDR_HPP
