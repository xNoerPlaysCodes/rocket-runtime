#include <crashdump.hpp>
#include <cstdio>
#include <cstring>
#include <format>
#include <iomanip>
#include <rocket/memory.hpp>
#include <sstream>
#include <util.hpp>
#include <native.hpp>
#include <rocket/macros.hpp>

#include <stacktrace>

namespace rocket {
    thread_local frame_allocator_t *char_allocator;
    alignas(frame_allocator_t) thread_local uint8_t _char_allocator_buf[sizeof(frame_allocator_t)] = {};
    thread_local bool allocator_initialized = false;

    void init_allocator() {
        if (allocator_initialized) return;
        char_allocator = (frame_allocator_t*) _char_allocator_buf;
        util::membuf_t *buf = util::get_memory_buffer();
        char_allocator->buffer = (uint8_t*) buf->mem;
        char_allocator->ogbuffer = (uint8_t*) buf->mem;
        char_allocator->size = buf->sz;
        char_allocator->ownership = false;
        allocator_initialized = true;
    }

    char* get_time_string() {
        std::time_t t = std::time(nullptr);
        std::tm tm{};

        native_gmtime(&t, &tm);

        static char *buf = (char*) char_allocator->allocate(64);
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

    int construct_stack_trace(char* buf, size_t len) {
#ifdef ROCKETGE__Platform_UnixCompatible
        if (len == 0) return 0;

        auto st = std::stacktrace::current();
        size_t st_size = st.size();

        if (st_size > 1000) {
            return std::snprintf(buf, len, "Stack trace too big (%zu > 1000) (Stack overflow?)\n", st_size);
        }

        // compute width for index column
        int index_width = 1;
        if (st_size >= 10)   index_width = 2;
        if (st_size >= 100)  index_width = 3;
        if (st_size >= 1000) index_width = 4;

        size_t remaining = len;
        char* out = buf;
        int total_written = 0;

        for (size_t i = 0; i < st_size; ++i) {
            const auto& frame = st[i];

            int written;
            if (frame.source_file().size() > 0) {
                written = std::snprintf(
                    out,
                    remaining,
                    "%*zu. %s:%u\n",

                    index_width, i,
                    frame.source_file().c_str(),
                    static_cast<unsigned>(frame.source_line())
                );
            } else {
                written = std::snprintf(
                    out,
                    remaining,
                    "%*zu. ???\n",

                    index_width, i
                );
            }

            if (written < 0) break;  // encoding error

            if (static_cast<size_t>(written) >= remaining) {
                // truncated — clamp and stop
                total_written += remaining - 1;
                break;
            }

            out += written;
            remaining -= written;
            total_written += written;
        }

        return total_written;

#else
        return std::snprintf(buf, len, "Platform doesn't support std::stacktrace\n");
#endif
    }

    char* crash_signal(bool fatal, void *mem_addr, const char *signal, const char *message) {
        init_allocator();
        char mem_addr_str[128] = {};
        if (mem_addr == nullptr) {
            std::snprintf(mem_addr_str, 128, "0x0");
        } else {
#ifdef ROCKETGE__Platform_Windows
            std::snprintf(mem_addr_str, 128, "0x%p", mem_addr);
#else
            std::snprintf(mem_addr_str, 128, "%p", mem_addr);
#endif
        }

        if (util::get_memory_buffer()->mem == nullptr) {
            static char buf[1024] = {};
            std::memset(buf, 0, 1024);

            if (std::strlen(message) + std::strlen(signal) > 512) {
                std::snprintf(buf, 1024, "%s", "OOM! Not enough memory to write crash dump");
                return buf;
            }

            std::snprintf(buf, 1024,
                "(Reduced crash dump)\n"
                "Signal|Message|MemAddr\n"
                "%s\n"
                "%s\n"
                "%s",

                signal, message, mem_addr_str
            );
            return buf;
        }

        constexpr size_t size = 1 * 1024 * 1024;
        char *buf = (char*) char_allocator->allocate(size);
        std::memset(buf, 0, size);
        int written = std::snprintf(buf, size,
            (fatal) ? ">- RocketGE has crashed! -<\n" : ""
            "Generated on %s\n"
            "%s occurred.\n"
            "\n"
            "%s @ [%s]\n"
            "%s\n"
            "\n"
            "%s\n\n",

            get_time_string(),
            fatal ? "A fatal exception" : "An exception",
            signal, mem_addr_str,
            message,
            fatal ? "The program will now dump the stack trace" : ""
        );

        written += construct_stack_trace(buf + written, size - written);
        written += std::snprintf(buf + written, size - written, 
            "\n>- Please report with all details to the RocketGE Github Maintainer -<");
        return buf;
    }
}
