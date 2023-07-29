#pragma once

#include <array>
#include <cstdint>
#include <exception>
#include <memory>
#include <queue>
#include <stdexcept>
#include <vector>

namespace easykey
{
/**
 * For now, there is no need to make it thread safe, because we will have one
 * single thread
 */
class ByteBuffer
{
  public:
    std::uint32_t get_integer4();
    std::uint8_t get_integer1();
    std::vector<std::uint8_t> get_next(const std::uint32_t size);
    void put_integer1(const std::uint8_t value);
    void put_integer4(const std::uint32_t value);
    void put_array(const std::uint8_t* array, const std::uint32_t size);
    bool has_content() const;
    std::uint32_t size() const;

  private:
    /**
     * Someone reads from an end(pop) and other one writes to the other
     * end(push)
     */
    std::queue<std::uint8_t> buffer;
};

class EmptyBufferException : public std::runtime_error
{
  private:
    const char* message;

  public:
    EmptyBufferException(const std::string message);
    virtual const char* what() const noexcept override;
};

};  // namespace easykey