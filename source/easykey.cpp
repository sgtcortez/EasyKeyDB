#include "easykey.hpp"
#include "know_nothing.hpp"

#include <array>
#include <bits/stdint-uintn.h>
#include <cstdint>
#include <regex>
#include <string>

using namespace easykey;
using namespace std;
using namespace knownothing;

static const string regex_value = "[^a-zA-Z0-9_]+";
static const regex invalid_key_regex(regex_value);

static bool is_key_valid(const string &key) {
  return !key.empty() && !regex_search(key, invalid_key_regex);
}

RequestMessage RequestMessage::read(uint8_t *input) {

  /**
   * First byte is the protocol version
   */
  const auto protocol = static_cast<knownothing::Protocol>(input[0]);

  /**
   * Next four bytes are the bytes that compose the first message 4 byte size
   */
  const auto first_message_size =
      deserialize({input[1], input[2], input[3], input[4]});

  /**
   * From the fifth byte until first messge size, its the first message
   * https://cplusplus.com/reference/string/string/string/
   */
  const auto first_message = string(((char *)input) + 5, first_message_size);

  /**
   * Check if the first message is a valid key value
   */
  if (!is_key_valid(first_message)) {
    throw "The key: \"" + first_message + "\" does not follow the regex: \"" +
        regex_value + "\"";
  }

  /**
   * After the first message, there are four bytes that compose the second
   * message size
   */
  const auto second_message_size = deserialize(
      {input[5 + first_message_size], input[5 + first_message_size + 1],
       input[5 + first_message_size + 2], input[5 + first_message_size + 3]});

  /**
   * Creates the vector for the second message.
   * NOTE: This is just a raw byte array, so no data is checked!
   */
  vector<uint8_t> second_message;

  // reserve the desired bytes for the message
  second_message.reserve(second_message_size);

  // pointer arithmetic to point to the beginning of the second message
  uint8_t *second_message_array = input + (5 + first_message_size + 4);

  for (uint32_t index = 0; index < second_message_size; index++) {
    second_message.push_back(second_message_array[index]);
  }

  return {protocol, first_message, second_message};
}

const vector<uint8_t> ResponseMessage::write() const {
  vector<uint8_t> response;

  // put the know nothing protocol version at the first byte
  // byte 0
  response.push_back(kn_protocol);

  // status code uses only one byte of the four bytes size limit
  // bytes 1 - 4
  response.push_back(1);
  response.push_back(0);
  response.push_back(0);
  response.push_back(0);

  // add the status code response
  // byte 5
  response.push_back(status_code);

  // serialize the value size
  const auto value_size = serialize(value.size());

  // append the 4 byte value size
  // byte 6 - 9
  for (const auto &b : value_size) {
    response.push_back(b);
  }

  // byte 10 - N
  // append the value
  for (const auto &ele : value) {
    response.push_back(ele);
  }

  return response;
}
