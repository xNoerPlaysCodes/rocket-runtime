#include "../include/io.hpp"
#include "util.hpp"

namespace rocket {
    namespace io {
        bool keystate_t::pressed() const { return current && !previous; }
        bool keystate_t::released() const { return !current && previous; }
        bool keystate_t::down() const { return current; }

        void add_listener(std::function<void(key_event_t)> listener) {
            ::util::key_listeners().push_back(listener);
        }

        void add_listener(std::function<void(mouse_event_t)> listener) {
            ::util::mouse_listeners().push_back(listener);
        }

        void add_listener(std::function<void(mouse_move_event_t)> listener) {
            ::util::mouse_move_listeners().push_back(listener);
        }
    }
}
