#ifndef ROCKETGE__GLFNLDR_HPP
#define ROCKETGE__GLFNLDR_HPP

#include <rocket/macros.hpp>
#include <vector>

#ifndef ROCKETGE__GLFNLDR_BACKEND_ENUM
#define ROCKETGE__GLFNLDR_BACKEND_ENUM ::glfnldr::backend_t::glad
#endif

namespace glfnldr {
    enum class backend_t : int {
        null = 0,
        glad,
        glew,
        libepoxy,
        android
    };

    bool init(backend_t backend);
    std::vector<backend_t> get_backends();
}

#endif//ROCKETGE__GLFNLDR_HPP
