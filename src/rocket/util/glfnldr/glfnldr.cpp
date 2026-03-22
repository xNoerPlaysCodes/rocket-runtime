#include "glfnldr.hpp"
#include "rocket/macros.hpp"

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
        if (b == backend_t::glew)
            return glew::init();

        if (b == backend_t::glad)
            return glad::init();

        if (b == backend_t::libepoxy)
            return libepoxy::init();
#else
        if (b == backend_t::android)
            return android::init();
#endif

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
