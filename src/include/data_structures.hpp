#ifndef ROCKETGE__DATA_STRUCTURES_HPP
#define ROCKETGE__DATA_STRUCTURES_HPP

#include <cstddef>
#include <cstdint>
#include <vector>
namespace rocket {
    /// @brief A Compressed Array in Memory
    class compressed_data_t {
    private:
        std::vector<uint8_t> data;
    public:
        /// @brief Set the data
        void set(std::vector<uint8_t> data);
        /// @brief Set the data
        void set(uint8_t *buffer, size_t sz);

        /// @brief Get the data
        std::vector<uint8_t> get();
        /// @brief Get the data
        uint8_t* get(size_t *size);
    public:
        compressed_data_t();
        compressed_data_t(std::vector<uint8_t>);
        compressed_data_t(uint8_t *buffer, size_t size);
    public:
        ~compressed_data_t();
    };
}

#endif//ROCKETGE__DATA_STRUCTURES_HPP
