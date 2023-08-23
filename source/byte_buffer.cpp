#include "byte_buffer.hpp"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <stdexcept>

using namespace std;
using namespace easykey;

ByteBuffer::ByteBuffer(const function<vector<uint8_t>(void)> source_trigger)
    : source_trigger(source_trigger)
{
}

uint32_t ByteBuffer::size() const
{
    return buffer.size();
}

uint8_t ByteBuffer::get_integer1()
{
    ensure_has_requested(1);
    auto value = buffer.front();
    buffer.pop();
    return value;
}

uint32_t ByteBuffer::get_integer4()
{
    ensure_has_requested(4);
    uint32_t value = 0;
    for (uint8_t index = 0; index < 4; index++)
    {
        auto temp = buffer.front();
        buffer.pop();
        value |= temp << (8 * index);
    }
    return value;
}

vector<uint8_t> ByteBuffer::get_next(const uint32_t size)
{
    ensure_has_requested(size);
    vector<uint8_t> output;
    output.reserve(min((uint64_t)size, buffer.size()));
    for (uint32_t index = 0; index < size && !buffer.empty(); index++)
    {
        auto temp = buffer.front();
        buffer.pop();
        output.push_back(temp);
    }
    return output;
}

bool ByteBuffer::has_content() const
{
    return !buffer.empty();
}

void ByteBuffer::ensure_has_requested(const uint32_t size)
{
    if (buffer.size() >= size)
    {
        // No need to request more data to the source trigger
        return;
    }

    for (const auto& value : source_trigger())
    {
        buffer.push(value);
    }

    // After triggering the source, still, there are no bytes available to
    // satisfy the requested amount
    if (buffer.size() < size)
    {
        const auto msg =
            "Requested amount exceed the available in the buffer ...! Buffer "
            "size: " +
            to_string(buffer.size()) + " requested: " + to_string(size);
        cerr << msg << endl;
        throw EmptyBufferException(msg);
    }
}

EmptyBufferException::EmptyBufferException(const string message)
    : std::runtime_error(message.c_str()), message(message.c_str())
{
}

const char* EmptyBufferException::what() const noexcept
{
    return message;
}