#include <internal_types.hpp>

namespace rocket {
    void native_window_t::set_instance(native_window_t *window) {
        native_window_t::bound = window;
    }
    native_window_t *native_window_t::get_instance() {
        return native_window_t::bound;
    }

    native_window_t *native_window_t::bound = nullptr;
}
