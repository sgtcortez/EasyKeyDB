#pragma once

#include <array>
#include <cstdint>
#include <exception>
#include <memory>
#include <queue>
#include <stdexcept>
#include <vector>
#include <functional>

namespace easykey
{
/**
 * For now, there is no need to make it thread safe, because we will have one
 * single thread
 */
class ByteBuffer
{
  public:
    ByteBuffer(
        const std::function<std::vector<std::uint8_t>(void)> source_trigger);

    std::uint32_t get_integer4();
    std::uint8_t get_integer1();
    std::vector<std::uint8_t> get_next(const std::uint32_t size);
    bool has_content() const;
    std::uint32_t size() const;

  private:
    /**
     * Someone reads from an end(pop) and other one writes to the other
     * end(push)
     */
    std::queue<std::uint8_t> buffer;

    /**
     * If buffer is empty, or, the client requested more bytes than the
     * available this function is executed
     */
    const std::function<std::vector<std::uint8_t>(void)> source_trigger;

    /**
     * Trigger source_trigger function and append data to the internal buffer
     */
    void ensure_has_requested(const std::uint32_t bytes_needed);
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