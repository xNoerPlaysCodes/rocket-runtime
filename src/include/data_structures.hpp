#ifndef ROCKETGE__DATA_STRUCTURES_HPP
#define ROCKETGE__DATA_STRUCTURES_HPP

#include <cstddef>
#include <cstdint>
#include <vector>
namespace rocket {
    /// @brief A Compressed Array in Memory
    class compressed_data_t {
    private:
        /// @brief Compressed Data
        std::vector<uint8_t> data;

        size_t original_size = 0;
    public:
        /// @brief Set the data
        bool set(const std::vector<uint8_t> &data);
        /// @brief Set the data
        bool set(uint8_t *buffer, size_t sz);

        /// @brief Get the data
        std::vector<uint8_t> get();
        /// @brief Get the data
        uint8_t* get(size_t *size);
    public:
        compressed_data_t();
        explicit compressed_data_t(const std::vector<uint8_t> &);
        compressed_data_t(uint8_t *buffer, size_t size);
    public:
        ~compressed_data_t();
    };
}

#endif//ROCKETGE__DATA_STRUCTURES_HPP
