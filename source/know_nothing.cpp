#include "know_nothing.hpp"

#include <cstdint>

using namespace std;
using namespace knownothing;

uint32_t knownothing::deserialize(array<uint8_t, 4> array) {
  uint32_t value = 0;
  value |= array[0];
  // there is a need to force the cast, otherwise, if the do the
  // left shift, we could lost some bits due to overflow
  value |= (uint32_t)array[1] << 8;
  value |= (uint32_t)array[2] << 16;
  value |= (uint32_t)array[3] << 24;
  return value;
}

array<uint8_t, 4> knownothing::serialize(uint32_t number) {
  array<uint8_t, 4> array;
  array[0] = number;
  array[1] = number >> 8;
  array[2] = number >> 16;
  array[3] = number >> 24;

  return array;
}
