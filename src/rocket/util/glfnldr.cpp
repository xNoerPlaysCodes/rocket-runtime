#include "glfnldr.hpp"
void BKEND_init();
#ifdef ROCKETGE__GLFNLDR_BACKEND_GLEW
#include <GL/glew.h>
#include <rocket/runtime.hpp>
void BKEND_init() {
    glewExperimental = true;
    glewInit();

    rocket::log("BACKEND GLEW INIT", "glfnldr", "BKEND_init", "debug");
}
#endif
#ifdef ROCKETGE__GLFNLDR_BACKEND_LIBEPOXY
#include <epoxy/gl.h>
#include <rocket/runtime.hpp>
void BKEND_init() {
    rocket::log("BACKEND LIBEPOXY INIT", "glfnldr", "BKEND_init", "debug");
}
#endif

namespace glfnldr {
    bool init(backend_t b) {
        if (b == backend_t::null || b == backend_t::glfw) {
            return false;
        }

        if (b == backend_t::glew) {
            BKEND_init();

            return true;
        }

        if (b == backend_t::libepoxy) {
            BKEND_init();

            return false; // FIXME Libepoxy causes segfault
        }

        return false;
    }
}
