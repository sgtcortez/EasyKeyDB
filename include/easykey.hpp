#pragma once

#include "know_nothing.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace easykey {

struct RequestMessage {
public:
  /**
   * Constructs the Message object
   * NOTE: The @param input must be a valid Know Nothing protocol message,
   * otherwise, we will have an undefined behavior
   */
  static RequestMessage read(std::vector<std::uint8_t> input);
  const knownothing::Protocol kn_protocol;
  const std::string key;
  const std::vector<std::uint8_t> value;
};

struct ResponseMessage {
public:
  enum StatusCode : std::uint8_t {
    OK = 0x01,
    CLIENT_ERROR = 0x02,
    SERVER_ERROR = 0x03,
  };
  const std::vector<std::uint8_t> write() const;
  const knownothing::Protocol kn_protocol;
  const StatusCode status_code;
  const std::vector<std::uint8_t> value;
};

}; // namespace easykey