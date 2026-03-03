#ifdef ROCKETGE__GLFNLDR_BACKEND_GLEW
#include <GL/glew.h>
#include <rocket/runtime.hpp>
bool BKEND_init() {
    glewExperimental = true;
    GLenum status = glewInit();

    rocket::log("glew initialized", "glfnldr", "BKEND_init", "debug");
    return status == GLEW_OK;
}
#endif
