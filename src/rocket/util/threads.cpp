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

    void thread_t::start() {
        uint64_t thread_id = thread_id_gen();
        auto main_ctx = rgl::get_main_context();

#ifdef ROCKETGE__Platform_Desktop

        if (this->ctx != nullptr) {
            if (this->ctx->w != nullptr && this->ctx->backend == window_backend_t::glfw) {
                glfwDestroyWindow((GLFWwindow*) this->ctx->w);
            }
        }
        this->ctx = new native_window_t;
        if (this->ctx->backend != window_backend_t::glfw && main_ctx->backend != window_backend_t::glfw) {
            rocket::log("only GLFW backend can have GL threads", "thread_t", "start", "fixme");
            return;
        }
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        this->ctx->w = (GLFWwindow*) glfwCreateWindow(1, 1, ("rgthread_" + std::to_string(thread_id)).c_str(), nullptr, (GLFWwindow*) main_ctx->w);
        std::thread t([ctx=this->ctx, fn=this->fn]() {
            glfwMakeContextCurrent((GLFWwindow*) ctx->w);
            rgl::init_gl_wtd();

            fn();

            glFlush();
            glFinish();
            glfwMakeContextCurrent(nullptr);
        });

        t.detach();
#endif
    }

    r_static uint64_t thread_t::get_thread_id() {
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
