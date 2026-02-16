#include <crashdump.hpp>
#include <cstdio>
#include <cstring>
#include <format>
#include <iomanip>
#include <sstream>
#include <util.hpp>
#include <native.hpp>

namespace rocket {
    template<typename T>
    struct emerg_alloc {
        using value_type = T;

        emerg_alloc() = default;

        template<class U>
        constexpr emerg_alloc(const emerg_alloc<U>&) {}

        T* allocate(std::size_t n) {
            auto* buf = util::get_memory_buffer();

            size_t bytes = n * sizeof(T);

            uintptr_t curr = (uintptr_t)buf->mem;
            uintptr_t aligned =
                (curr + alignof(T) - 1) & ~(uintptr_t(alignof(T) - 1));

            uintptr_t next = aligned + bytes;
            uintptr_t end  = (uintptr_t)buf->mem + buf->sz;

            if (next > end) {
                rnative::exit_now(234);
            }

            buf->mem = (void*)next;
            return (T*)aligned;
        }

        void deallocate(T* p, std::size_t) noexcept {
            ::operator delete(p);
        }
    };

    thread_local emerg_alloc<char> char_allocator;

    char* get_time_string() {
        std::time_t t = std::time(nullptr);
        std::tm tm{};

        native_gmtime(&t, &tm);

        static char *buf = char_allocator.allocate(64);
        for (int i = 0; i < 64; ++i) buf[i] = 0;

        int written = std::snprintf(
            buf, 64,
            "%04d-%02d-%02d %02d:%02d:%02d",
            tm.tm_year + 1900,
            tm.tm_mon + 1,
            tm.tm_mday,
            tm.tm_hour,
            tm.tm_min,
            tm.tm_sec
        );
        return buf;
    }

    char* crash_signal(bool fatal, void *mem_addr, const char *signal, const char *message) {
        if (util::get_memory_buffer()->mem == nullptr) {
            static char buf[1024] = {};
            for (int i = 0; i < 1024; ++i) buf[i] = 0;

            if (std::strlen(message) + std::strlen(signal) > 512) {
                std::snprintf(buf, 1024, "%s", "OOM! Not enough memory to write crash dump");
                return buf;
            }

            std::snprintf(buf, 1024,
                "(Reduced crash dump)\n"
                "Signal|Message|MemAddr\n"
                "%s\n"
                "%s\n"
                "%p",

                signal, message, mem_addr
            );
            return buf;
        }

        constexpr size_t size = 4 * 1024 * 1024;
        char *buf = char_allocator.allocate(size);
        std::memset(buf, 0, size);
        std::snprintf(buf, size,
            "Generated on %s\n"
            "%s occurred.\n"
            "\n"
            "%s @ [%p]\n"
            "%s\n"
            "\n"
            "%s",

            get_time_string(),
            fatal ? "A fatal exception" : "An exception",
            signal, mem_addr,
            message,
            fatal ? "The program will now exit" : ""
        );
        return buf;
    }
}
