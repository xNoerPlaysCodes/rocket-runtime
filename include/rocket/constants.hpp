#ifndef ROCKETGE__CONSTANTS_HPP
#define ROCKETGE__CONSTANTS_HPP

#include <limits>
#include "types.hpp"

/// @brief Constants
namespace rocket::cst {
    constexpr int failure = -1;
    constexpr uint8_t unknown_u8 = std::numeric_limits<uint8_t>::max();
    constexpr uint16_t unknown_u16 = std::numeric_limits<uint16_t>::max();
    constexpr uint32_t unknown_u32 = std::numeric_limits<uint32_t>::max();
    constexpr uint64_t unknown_u64 = std::numeric_limits<uint64_t>::max();

    constexpr int8_t unknown_i8 = std::numeric_limits<int8_t>::max();
    constexpr int16_t unknown_i16 = std::numeric_limits<int16_t>::max();
    constexpr int32_t unknown_i32 = std::numeric_limits<int32_t>::max();
    constexpr int64_t unknown_i64 = std::numeric_limits<int64_t>::max();

    /// @brief rocket::io magic number to set [var: vec2] to
    ///         current mouse position
    constexpr float io_mn_set_to_current_mpos = 1.7186765875;
}

#endif // ROCKETGE__CONSTANTS_HPP
