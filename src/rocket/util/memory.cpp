#include <rocket/memory.hpp>
#include <intl_macros.hpp>

namespace rocket {
    frame_allocator_t::frame_allocator_t(uint8_t *buffer, size_t sz) {
        this->buffer = buffer;
        this->ogbuffer = buffer;
        this->size = sz;
        this->ownership = false;
    }

    frame_allocator_t::frame_allocator_t(size_t sz) {
        this->buffer = new uint8_t[sz];
        this->ogbuffer = buffer;
        this->size = sz;
        this->ownership = true;
    }

    static size_t aligned(size_t sz, size_t A /* Alignment */) {
        return (sz + (A - 1)) & ~(A - 1);
    }

    uint8_t* frame_allocator_t::allocate(size_t size, size_t alignment) {
        uintptr_t current = reinterpret_cast<uintptr_t>(this->buffer);
        uintptr_t aligned_ptr = aligned(current, alignment);
        uintptr_t new_ptr = aligned_ptr + size;

        r_assert(new_ptr - reinterpret_cast<uintptr_t>(this->ogbuffer) <= this->size);

        this->buffer = reinterpret_cast<uint8_t*>(new_ptr);

        return reinterpret_cast<uint8_t*>(aligned_ptr);
    }

    void frame_allocator_t::clear() {
        this->buffer = ogbuffer;
    }

    frame_allocator_t::~frame_allocator_t() {
        if (this->ownership) {
            delete[] this->ogbuffer;
            this->ogbuffer = nullptr;
            this->buffer = nullptr;
        }
    }
}
