#ifndef ROCKETGE__THREADS_HPP
#define ROCKETGE__THREADS_HPP

#include <GLFW/glfw3.h>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
namespace rocket {
    class thread_t {
    private:
        std::function<void(void *arg1, void *arg2, void *arg3)> fn;
        GLFWwindow *ctx;
        void *arg1, *arg2, *arg3;
    public:
        /// @brief Schedules these calls to be run on the main thread at frame-end
        /// @note Thread-Safe
        static void schedule(std::function<void()> fn);

        /// @brief Get the thread ID (64-bit integer)
        static uint64_t get_thread_id();
    public:
        /// @brief Starts the thread
        /// @note [FIXME] DOESNT WORK
        void start();
    public:
        thread_t(std::function<void(void *arg1, void *arg2, void *arg3)> fn, void *arg1 = nullptr, void *arg2 = nullptr, void *arg3 = nullptr);
    public:
        ~thread_t();
    };
}

#endif//ROCKETGE__THREADS_HPP
