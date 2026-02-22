#include <cstdint>
#include <data_structures.hpp>
#include <rocket/runtime.hpp>
#include <vector>
#include <lzf.h>

namespace rocket {
    compressed_data_t::compressed_data_t() = default;

    compressed_data_t::compressed_data_t(const std::vector<uint8_t> &data) {
        this->set(data);
    }

    compressed_data_t::compressed_data_t(uint8_t *buffer, size_t sz) {
        this->set(buffer, sz);
    }

    bool compressed_data_t::set(const std::vector<uint8_t> &) {
        original_size = data.size();
        this->data.resize(original_size * 2); // extra space
        size_t written = lzf_compress(data.data(), data.size(), this->data.data(), this->data.size());

        if (written == 0) {
            rocket::log("Compression failed", "compressed_data_t", "set", "error");
            return false;
        }

        this->data.resize(written);

        return true;
    }

    bool compressed_data_t::set(uint8_t *buffer, size_t sz) {
        original_size = sz;
        this->data.resize(original_size * 2); // extra space
        size_t written = lzf_compress(buffer, sz, this->data.data(), this->data.size());

        if (written == 0) {
            rocket::log("Compression failed", "compressed_data_t", "set", "error");
            return false;
        }

        this->data.resize(written);

        return true;
    }

    std::vector<uint8_t> compressed_data_t::get() {
        return this->data;
    }

    uint8_t* compressed_data_t::get(size_t *sz) {
        *sz = this->data.size();
        return this->data.data();
    }

    compressed_data_t::~compressed_data_t() = default;
}
