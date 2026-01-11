#include <cstdint>
#include <functional>
#include <sstream>
#include <thread>
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

    thread_t::thread_t(std::function<void(void *arg1, void *arg2, void *arg3)> fn, void *arg1, void *arg2, void *arg3) {
        if (fn == nullptr) {
            rocket::log_error("native thread must be valid ptr", "thread_t::thread_t", "error");
            return;
        }
        this->fn = fn;
        this->arg1 = arg1;
        this->arg2 = arg2;
        this->arg3 = arg3;
    }

    RGE_STATIC_FUNC_IMPL void thread_t::schedule(std::function<void()> fn) {
        rgl::schedule_gl(fn);
    }

    RGE_STATIC_FUNC_IMPL void thread_t::run(std::function<void()> fn) {
        std::thread t([&fn]() {
            fn();
        });
        t.detach();
    }

    void thread_t::start() {
        uint64_t thread_id = thread_id_gen();
        auto main_ctx = rgl::__rglexp_get_main_context();
        
        rocket::log_error("[fixme] does not work", "thread_t::start", "fixme");

        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        this->ctx = glfwCreateWindow(1, 1, ("rgthread_" + std::to_string(thread_id)).c_str(), nullptr, main_ctx);
        std::thread t([ctx=this->ctx, fn=this->fn, arg1=this->arg1, arg2=this->arg2, arg3=this->arg3]() {
            glfwMakeContextCurrent(ctx);
            rgl::init_gl_wtd();

            fn(arg1, arg2, arg3);

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
