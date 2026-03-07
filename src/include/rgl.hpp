#include "rocket/rgl.hpp"

#ifndef RocketGL__INT_HPP
#define RocketGL__INT_HPP

namespace rocket {
    struct native_window_t;
}

namespace rgl {
    rocket::native_window_t *get_main_context();
    void schedule_gl(std::function<void()> fn);
    void cleanup_all();
    void run_all_scheduled_gl();
}

#endif//RocketGL__INT_HPP
