#ifndef ROCKETGE__THREADS_HPP
#define ROCKETGE__THREADS_HPP

#include <cstdint>
#include <functional>
#include <chrono>
#include <string>

namespace rocket {
    struct native_window_t;
    class thread_t {
    private:
    public:
        /// @brief Schedules these calls to be run on the main thread at frame-end
        /// @note Thread-Safe
        static void schedule(std::function<void()> fn);
        /// @brief Schedules these calls to be run on a NEW thread after time has passed
        static void schedule_async(std::function<void()> fn, std::chrono::milliseconds start_after);
        /// @brief Runs them on a thread NOW
        static void run(std::function<void()> fn);
        /// @brief Get the thread ID (64-bit integer)
        static uint64_t get_thread_id();
        /// @brief Set the thread name for current thread
        /// @note On Linux MAX 15 chars only
        static void set_thread_name(std::string name);
    public:
        thread_t() = default;
        ~thread_t() = default;
    };
}

#endif//ROCKETGE__THREADS_HPP
