#ifndef ROCKETGE__MEMORY_HPP
#define ROCKETGE__MEMORY_HPP

#include <cstddef>
#include <cstdint>
namespace rocket {
    class frame_allocator_t {
    private:
        uint8_t *buffer = nullptr;
        uint8_t *ogbuffer = nullptr;
        size_t size = 0;
        bool ownership = false;

        friend void init_allocator();
    public:
        uint8_t *allocate(size_t size, size_t alignment = 32);
        void clear();
    public:
        /// @brief Create a frame allocator from a premade buffer
        frame_allocator_t(uint8_t *buffer, size_t sz);
        /// @brief Create a frame allocator from malloc'd buffer
        frame_allocator_t(size_t sz);
    public:
        ~frame_allocator_t();
    };
}

#endif//ROCKETGE__MEMORY_HPP
