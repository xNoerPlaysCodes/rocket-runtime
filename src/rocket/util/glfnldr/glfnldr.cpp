#include "glfnldr.hpp"

bool BKEND_init();
bool BKEND_glad_init();

namespace glfnldr {
    bool init(backend_t b) {
        if (b == backend_t::null) {
            return false;
        }

        if (b == backend_t::glew) {
            return BKEND_init();
        }

        if (b == backend_t::glad) {
            return BKEND_glad_init();
        }

        return false;
    }

    std::vector<glfnldr::backend_t> get_backends() {
        return {
            backend_t::null,
            backend_t::glad,
#ifdef ROCKETGE__GLFNLDR_BACKEND_GLEW
            backend_t::glew,
#endif
        };
    }
}
