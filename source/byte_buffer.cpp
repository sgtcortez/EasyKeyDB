#include "byte_buffer.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <bits/stdint-uintn.h>

using namespace std;
using namespace easykey;

void ByteBuffer::put_integer1(const uint8_t value)
{
    buffer.push(value);
}

void ByteBuffer::put_integer4(const uint32_t value)
{
    buffer.push(value);
    buffer.push(value >> 8);
    buffer.push(value >> 16);
    buffer.push(value >> 24);
}

void ByteBuffer::put_array(const uint8_t* array, const uint32_t size)
{
    for (uint32_t index = 0; index < size; index++)
    {
        buffer.push(array[index]);
    }
}

uint32_t ByteBuffer::size() const
{
    return buffer.size();
}

uint8_t ByteBuffer::get_integer1()
{
    if (buffer.empty())
    {
        return 0;
    }
    auto value = buffer.front();
    buffer.pop();
    return value;
}

uint32_t ByteBuffer::get_integer4()
{
    uint32_t value = 0;
    for (uint8_t index = 0; index < 4 && !buffer.empty(); index++)
    {
        auto temp = buffer.front();
        buffer.pop();
        value |= temp << (8 * index);
    }
    return value;
}

vector<uint8_t> ByteBuffer::get_next(const uint32_t size)
{
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