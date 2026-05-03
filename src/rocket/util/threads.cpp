#ifdef ROCKETGE__Platform_Desktop
#include <GLFW/glfw3.h>
#endif
#include <cstdint>
#include <functional>
#include <mutex>
#include <native.hpp>
#include <sstream>
#include <thread>
#include <vector>
#define RGL_EXPOSE_NATIVE_LIB
#include <rgl.hpp>
#include "rocket/runtime.hpp"
#include <rocket/threads.hpp>
#include "intl_macros.hpp"
#include "internal_types.hpp"

namespace rocket {
    r_static void thread_t::schedule(std::function<void()> fn) {
        rgl::schedule_gl(fn);
    }

    r_static void thread_t::schedule_async(std::function<void()> fn, std::chrono::milliseconds start_after) {
        std::thread([fn = std::move(fn), start_after]() {
            std::this_thread::sleep_for(start_after);
            fn();
        }).detach();
    }

    struct thread_pool_t {
        bool init = false;

        uint8_t count = 0;
        uint8_t in_use = 0;

        std::vector<std::thread> threads;
    };

    std::mutex pool_mutex;
    thread_pool_t pool;

    r_static void thread_t::run(std::function<void()> fn) {
        // {
        //     std::lock_guard<std::mutex> _(pool_mutex);
        //     if (!pool.init) {
        //         pool.init = true;
        //         pool.threads.resize(32);
        //         pool.count = 32;
        //         pool.in_use = 0;
        //     }
        //
        //     if (pool.in_use == pool.count) {
        //         while (true) {}
        //     }
        //
        //     pool.in_use++;
        // }

        std::thread(fn).detach();
    }

    r_static void thread_t::set_thread_name(std::string name) {
        rnative::set_thread_name(name.c_str());
    }

    r_static uint64_t thread_t::get_thread_id() {
        std::stringstream ss;
        ss << std::this_thread::get_id();
        return std::stoull(ss.str());
    }
}
