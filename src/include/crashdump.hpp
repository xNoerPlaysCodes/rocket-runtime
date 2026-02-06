#ifndef ROCKETGE__CRASHDUMP_HPP
#define ROCKETGE__CRASHDUMP_HPP

#include <string>
namespace rocket {
    struct crash_info_t {};

    char* crash_signal(bool fatal, void *mem_addr, const char *signal, const char *message);
}

#endif//ROCKETGE__CRASHDUMP_HPP
