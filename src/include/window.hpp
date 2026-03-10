#ifndef ROCKETGE_INT__WINDOW_HPP
#define ROCKETGE_INT__WINDOW_HPP

#include <functional>
#include <rocket/glfnldr.hpp>
#include <rocket/io.hpp>
#include <string>
#include <rocket/window_helpers.hpp>

namespace rocket {
    namespace window {
        void null_cpl_init();
        void glfw_cpl_init();
        void embedded_cpl_init();
    }
}

#endif
