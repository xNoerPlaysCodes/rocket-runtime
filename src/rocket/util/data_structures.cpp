#include <algorithm>
#include <cstdint>
#include <data_structures.hpp>
#include <rocket/runtime.hpp>
#include <vector>

extern "C" {
    #include <lib/lzf/lzf.h>
}

namespace rocket {
    compressed_data_t::compressed_data_t() = default;

    compressed_data_t::compressed_data_t(const std::vector<uint8_t> &data) {
        this->set(data);
    }

    compressed_data_t::compressed_data_t(uint8_t *buffer, size_t sz) {
        this->set(buffer, sz);
    }

    bool compressed_data_t::set(const std::vector<uint8_t> &set_data) {
        original_size = set_data.size();
        this->data.resize(std::max(original_size * 2, (size_t)32)); // extra space
        size_t written = ::lzf_compress(set_data.data(), set_data.size(), this->data.data(), this->data.size());

        if (written == 0) {
            rocket::log("Compression failed", "compressed_data_t", "set", "error");
            return false;
        }

        this->data.resize(written);

        return true;
    }

    bool compressed_data_t::set(uint8_t *buffer, size_t sz) {
        original_size = sz;
        // we have to do this because:
        // long win32 x86_64: 32 bit
        // long unix  x86_64: 64 bit
        this->data.resize(std::max(original_size * 2, (size_t)32)); // extra space
        size_t written = ::lzf_compress(buffer, sz, this->data.data(), this->data.size());

        if (written == 0) {
            rocket::log("Compression failed", "compressed_data_t", "set", "error");
            return false;
        }

        this->data.resize(written);

        return true;
    }

    std::vector<uint8_t> compressed_data_t::get() {
        std::vector<uint8_t> decompressed_data;
        decompressed_data.resize(this->original_size);
        size_t written = ::lzf_decompress(this->data.data(), this->data.size(), decompressed_data.data(), decompressed_data.size());
        if (written == 0) {
            rocket::log("Decompression failed", "compressed_data_t", "get", "error");
            return {};
        }
        decompressed_data.resize(written); // safety measure

        return decompressed_data;
    }

    uint8_t* compressed_data_t::get(size_t &sz) {
        uint8_t *decompressed_data = new uint8_t[this->original_size];
        size_t written = ::lzf_decompress(this->data.data(), this->data.size(), decompressed_data, this->original_size);
        if (written == 0) {
            rocket::log("Decompression failed", "compressed_data_t", "get", "error");
            delete[] decompressed_data;
            sz = 0;
            return nullptr;
        }

        sz = written;

        return decompressed_data;
    }

    compressed_data_t::~compressed_data_t() = default;
}
