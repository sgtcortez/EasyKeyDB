#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace knownothing
{
/**
 * For now, we will deserialize only the 4 byte number
 */
std::uint32_t deserialize(std::array<std::uint8_t, 4> array);

/**
 * For now, we serialize only the 4 byte number
 */
std::array<std::uint8_t, 4> serialize(std::uint32_t number);

/**
 * The availables Know Nothing Protocol Versions
 */
enum Protocol : std::uint8_t
{
    V1 = 0x01,
};

bool check_integrity(std::vector<uint8_t> input);

};  // namespace knownothing