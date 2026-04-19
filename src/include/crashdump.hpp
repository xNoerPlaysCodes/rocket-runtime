#ifndef ROCKETGE__CRASHDUMP_HPP
#define ROCKETGE__CRASHDUMP_HPP

namespace rocket {
    struct crash_info_t {};

    /// @brief Invalidates previous crash messages on invocation
    char* crash_signal(bool fatal, void *mem_addr, const char *signal, const char *message);

    /// @brief Java-like stack trace + exit function
    [[noreturn]] void crash_with_stacktrace() noexcept;
}

#endif//ROCKETGE__CRASHDUMP_HPP
