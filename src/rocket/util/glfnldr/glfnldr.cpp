#include "glfnldr.hpp"
#include "rocket/macros.hpp"
#include <iostream>
#include <rocket/runtime.hpp>

namespace glfnldr {
    namespace android { bool init(); }
    namespace glew { bool init(); }
    namespace glad { bool init(); }
    namespace libepoxy { bool init(); }
}

namespace glfnldr {
    bool init(backend_t b) {
        if (b == backend_t::null)
            return false;

#ifdef ROCKETGE__Platform_Desktop
#ifdef ROCKETGE__GLFNLDR_BACKEND_GLEW
        if (b == backend_t::glew)
            return glew::init();
#else
        if (b == backend_t::glew)
            return false;
#endif

        if (b == backend_t::glad)
            return glad::init();

#ifdef ROCKETGE__GLFNLDR_BACKEND_LIBEPOXY
        if (b == backend_t::libepoxy)
            return libepoxy::init();
#else
        if (b == backend_t::libepoxy)
            return false;
#endif
#else
        if (b == backend_t::android)
            return android::init();
#endif

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
