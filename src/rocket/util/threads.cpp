#include <cstdint>
#include <functional>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>
#define RGL_EXPOSE_NATIVE_LIB
#include <rgl.hpp>
#include "rocket/runtime.hpp"
#include <rocket/threads.hpp>
#include "intl_macros.hpp"

namespace rocket {
    uint64_t thread_id_gen() {
        static uint64_t thread_id = 1;
        return thread_id++;
    }

    thread_t::thread_t(std::function<void()> fn) {
        if (fn == nullptr) {
            rocket::log("native thread must be valid ptr", "thread_t", "thread_t", "error");
            return;
        }
        this->fn = fn;
    }

    RGE_STATIC_FUNC_IMPL void thread_t::schedule(std::function<void()> fn) {
        rgl::schedule_gl(fn);
    }

    RGE_STATIC_FUNC_IMPL void thread_t::schedule_async(std::function<void()> fn, std::chrono::milliseconds start_after) {
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

    RGE_STATIC_FUNC_IMPL void thread_t::run(std::function<void()> fn) {
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

        std::thread t([&fn]() {
            fn();
        });
        t.detach();
    }

    void thread_t::start() {
        uint64_t thread_id = thread_id_gen();
        auto main_ctx = rgl::get_main_context();
        
        rocket::log("[fixme] does not work", "thread_t", "start", "fixme");

        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        this->ctx = glfwCreateWindow(1, 1, ("rgthread_" + std::to_string(thread_id)).c_str(), nullptr, main_ctx);
        std::thread t([ctx=this->ctx, fn=this->fn]() {
            glfwMakeContextCurrent(ctx);
            rgl::init_gl_wtd();

            fn();

            glFinish();
            glfwMakeContextCurrent(nullptr);
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        t.detach();
    }

    RGE_STATIC_FUNC_IMPL uint64_t thread_t::get_thread_id() {
        std::stringstream ss;
        ss << std::this_thread::get_id();
        return std::stoull(ss.str());
    }

    thread_t::~thread_t() {
        if (this->ctx != nullptr) {
            this->ctx = nullptr;
        }
    }
}
