#ifndef ROCKETGE__CRASHDUMP_HPP
#define ROCKETGE__CRASHDUMP_HPP

#include <string>
namespace rocket {
    struct crash_info_t {};

    std::string crash_signal(bool fatal, void *mem_addr, std::string signal, std::string message);
}

#endif//ROCKETGE__CRASHDUMP_HPP
