#ifndef ROCKETGE__GLFNLDR_HPP
#define ROCKETGE__GLFNLDR_HPP

#include <vector>
namespace glfnldr {
    enum class backend_t : int {
        null = 0,
        glew,
        libepoxy,
        glfw // Unsupported FIXME
    };

    std::vector<backend_t> get_backends();
}

#endif//ROCKETGE__GLFNLDR_HPP
