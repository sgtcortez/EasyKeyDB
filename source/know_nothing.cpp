#include "know_nothing.hpp"

#include <cstdint>
#include <iostream>

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

/**
 * This needs to be done, because the kernel has a single buffer for the file
 * descriptors, so it just stores data without limitations
 */
bool knownothing::check_integrity(vector<uint8_t> input) {
  if (input.size() < 3) {
    return false;
  }

  uint32_t current_position = 0;

  // For now, just the first protocol is supported!
  if (input[current_position++] != Protocol::V1) {
    cerr << "Only the first protocol is supported!" << endl;
    return false;
  }
  uint8_t messages_number = input[current_position++];
  if (messages_number == 0) {
    // At least, one message must be provided!
    cerr << "At least, one message must be provided!" << endl;
    return false;
  }

  // Validates the messages(size and the rovided at messages number)
  for (uint32_t index = 0; index < messages_number; index++) {

    if (current_position + 3 >= input.size()) {
      cerr << "Message is not formated properly!" << endl;
      return false;
    }

    uint32_t message_size =
        deserialize({input[current_position++], input[current_position++],
                     input[current_position++], input[current_position++]});

    // move to the next message size
    current_position += message_size;
  }
  // validates if the messages numbers and the messages with their sizes matches
  // with the input size
  const auto valid = current_position == input.size();
  if (!valid) {
    cerr << "Messages size do not follow the specificated size!" << endl;
  }

  return valid;
}
