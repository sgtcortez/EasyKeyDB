#pragma once

#include "byte_buffer.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace knownothing
{
/**
 * The availables Know Nothing Protocol Versions
 */
enum Protocol : std::uint8_t
{
    V1 = 0x01,
};
}  // namespace knownothing

namespace easykey
{
struct RequestMessage
{
  public:
    static std::unique_ptr<RequestMessage> read(easykey::ByteBuffer& buffer);
    const knownothing::Protocol kn_protocol;
    const std::string key;
    const std::vector<std::uint8_t> value;
};

struct ResponseMessage
{
  public:
    enum StatusCode : std::uint8_t
    {
        OK = 0x01,
        CLIENT_ERROR = 0x02,
        SERVER_ERROR = 0x03,
    };
    const std::vector<std::uint8_t> write() const;
    const knownothing::Protocol kn_protocol;
    const StatusCode status_code;
    const std::vector<std::uint8_t> value;
};

};  // namespace easykey