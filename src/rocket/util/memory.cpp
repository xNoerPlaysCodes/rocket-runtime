#include <rocket/memory.hpp>

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

    uint8_t *frame_allocator_t::allocate(size_t size, size_t alignment) {
        size = aligned(size, alignment);
        uint8_t *buf = this->buffer;
        this->buffer += size;
        return buf;
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
