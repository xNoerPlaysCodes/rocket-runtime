#ifndef ROCKETGE__INT_GLFNLDR_HPP
#define ROCKETGE__INT_GLFNLDR_HPP

#include <rocket/glfnldr.hpp>

namespace glfnldr {
    /// @brief (Try) to load OpenGL functions use a function loader backend
    bool init(backend_t backend);
}

#endif//ROCKETGE__INT_GLFNLDR_HPP
