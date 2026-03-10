#include "glfnldr.hpp"

bool BKEND_glew_init();
bool BKEND_glad_init();
bool BKEND_libepoxy_init();

namespace glfnldr {
    bool init(backend_t b) {
        if (b == backend_t::null)
            return false;

        if (b == backend_t::glew)
            return BKEND_glew_init();

        if (b == backend_t::glad)
            return BKEND_glad_init();

        if (b == backend_t::libepoxy)
            return BKEND_libepoxy_init();

        return false;
    }

    std::vector<glfnldr::backend_t> get_backends() {
        return {
            backend_t::null,
#ifdef ROCKETGE__GLFNLDR_BACKEND_GLEW
            backend_t::glew,
#endif
        };
    }
}
