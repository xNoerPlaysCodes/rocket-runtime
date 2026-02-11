#ifndef ROCKETGE__THREADS_HPP
#define ROCKETGE__THREADS_HPP

#include <GLFW/glfw3.h>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
namespace rocket {
    /// @brief A OpenGL-compatible thread
    /// @note Some static functions are not compatible with OpenGL calls
    class thread_t {
    private:
        std::function<void()> fn;
        GLFWwindow *ctx;
    public:
        /// @brief Schedules these calls to be run on the main thread at frame-end
        /// @note Thread-Safe
        static void schedule(std::function<void()> fn);
        /// @brief Runs them on a thread NOW
        /// @note Thread-Safe ONLY if NO OpenGL calls are used
        static void run(std::function<void()> fn);

        /// @brief Get the thread ID (64-bit integer)
        static uint64_t get_thread_id();
    public:
        /// @brief Starts the thread
        /// @note [FIXME] DOESNT WORK
        void start();
    public:
        thread_t(std::function<void()> fn);
    public:
        ~thread_t();
    };
}

#endif//ROCKETGE__THREADS_HPP
