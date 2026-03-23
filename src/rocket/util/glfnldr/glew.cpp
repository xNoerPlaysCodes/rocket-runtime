#ifdef ROCKETGE__GLFNLDR_BACKEND_GLEW
#include <GL/glew.h>
#include <rocket/runtime.hpp>
namespace glfnldr::glew {
    bool init() {
        glewExperimental = true;
        GLenum status = glewInit();

        rocket::log("glew initialized with exit code " + std::to_string(status), "glfnldr::glew", "init", "debug");
        return status == GLEW_OK;
    }
}
#endif
