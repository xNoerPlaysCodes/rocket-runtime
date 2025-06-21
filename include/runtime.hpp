#ifndef ROCKETGE__RUNTIME_HPP
#define ROCKETGE__RUNTIME_HPP

#include "types.hpp"
#include "window.hpp"

namespace rocket {
    #define ROCKETGE_MINOR_VERSION  "1"
    #define ROCKETGE_MAJOR_VERSION  "0"
    #define ROCKETGE_BUILD          "open-dev"
    #define ROCKETGE_VERSION        ROCKETGE_MAJOR_VERSION "." ROCKETGE_MINOR_VERSION "-" ROCKETGE_BUILD
}
#define ROCKETGE_OpenGL_SupportComputeShader

#endif
