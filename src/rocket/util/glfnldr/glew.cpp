#ifdef ROCKETGE__GLFNLDR_BACKEND_GLEW
// #include <GL/glew.h>
#define ROCKETGE__RUNTIME_SKIP_HEADER_INCLUSION
#include <rocket/runtime.hpp>
namespace glfnldr::glew {
    bool init() {
        // glewExperimental = true;
        // GLenum status = glewInit();
        //
        // rocket::log("glew initialized with exit code " + std::to_string(status), "glfnldr::glew", "init", "debug");
        // return status == GLEW_OK;
        return false;
    }
}
#endif
