#ifndef ROCKETGE__RUNTIME_HPP
#define ROCKETGE__RUNTIME_HPP

#include "asset.hpp"
#include "io.hpp"
#include "renderer.hpp"
#include "types.hpp"
#include "window.hpp"

namespace rocket {
    #define ROCKETGE_MINOR_VERSION  "1"
    #define ROCKETGE_MAJOR_VERSION  "0"
    #define ROCKETGE_BUILD          "open-dev"
    #define ROCKETGE_VERSION        ROCKETGE_MAJOR_VERSION "." ROCKETGE_MINOR_VERSION "-" ROCKETGE_BUILD
}
#define ROCKETGE_OpenGL_SupportComputeShader

namespace rocket {
    enum class log_level_t : int {
        warn = 1,
        fatal_to_function = 2,
        fatal = 3,
    };

    void set_log_level(log_level_t level);
}

#endif
