#include <crashdump.hpp>
#include <format>
#include <iomanip>
#include <sstream>

namespace rocket {
    std::string get_time_string() {
        std::time_t time = std::time(nullptr);
        std::tm *tm_info = std::localtime(&time);

        std::stringstream ss;
        ss << std::put_time(tm_info, "on %Y-%m-%d at %H:%M:%S");
        return ss.str();
    }

    std::string crash_signal(bool fatal, void *mem_addr, std::string signal, std::string message) {
        return std::format(R"(Generated {}
{} occurred.

{} @ [{}]
{}

{})",
                get_time_string(),
                fatal ? "A fatal exception" : "An exception",
                signal, mem_addr,
                message,
                fatal ? "The program will now exit" : "");
    }
}
