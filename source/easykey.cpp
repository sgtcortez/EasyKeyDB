#include "easykey.hpp"
#include "know_nothing.hpp"

#include <array>
#include <bits/stdint-uintn.h>
#include <cstdint>
#include <iostream>
#include <memory>
#include <regex>
#include <string>

using namespace easykey;
using namespace std;
using namespace knownothing;

static const string regex_value = "[^a-zA-Z0-9_]+";
static const regex invalid_key_regex(regex_value);

static bool is_key_valid(const string &key)
{
    return !key.empty() && !regex_search(key, invalid_key_regex);
}

unique_ptr<RequestMessage> RequestMessage::read(vector<uint8_t> input)
{
    if (!check_integrity(input))
    {
        // Would be nice to use optional ...
        return unique_ptr<RequestMessage>(nullptr);
    }

    uint32_t message_index = 0;

    /**
     * First byte is the protocol version
     */
    const auto protocol =
        static_cast<knownothing::Protocol>(input[message_index++]);

    const auto messages_number = input[message_index++];
    switch (messages_number)
    {
        case 1:
            // when is a read operation, only a message will be provided
        case 2:
            // when is a write operation, two messages will be provided
            break;
        default:
            cerr << "Invalid message! To read a message must be provided, to "
                    "write, "
                    "two messages must be provided!"
                 << endl;
            // Would be nice to use optional ...
            return unique_ptr<RequestMessage>(nullptr);
    }

    /**
     * Next four bytes are the bytes that compose the first message 4 byte size
     */
    const auto first_message_size = deserialize({input[message_index++],
                                                 input[message_index++],
                                                 input[message_index++],
                                                 input[message_index++]});

    /**
     * From the fifth byte until first messge size, its the first message
     * https://cplusplus.com/reference/string/string/string/
     */
    const auto first_message =
        string(((char *)input.data()) + message_index, first_message_size);

    message_index += first_message_size;

    /**
     * Check if the first message is a valid key value
     */
    if (!is_key_valid(first_message))
    {
        cerr << "The key: \"" + first_message +
                    "\" does not follow the regex: \"" + regex_value + "\""
             << endl;

        // Would be nice to use optional ...
        return unique_ptr<RequestMessage>(nullptr);
    }

    /**
     * After the first message, there are four bytes that compose the second
     * message size
     */
    const auto second_message_size = deserialize({input[message_index++],
                                                  input[message_index++],
                                                  input[message_index++],
                                                  input[message_index++]});

    /**
     * Creates the vector for the second message.
     * NOTE: This is just a raw byte array, so no data is checked!
     */
    vector<uint8_t> second_message;

    // reserve the desired bytes for the message
    second_message.reserve(second_message_size);

    // pointer arithmetic to point to the beginning of the second message
    uint8_t *second_message_array = input.data() + message_index;

    for (uint32_t index = 0; index < second_message_size; index++)
    {
        second_message.push_back(second_message_array[index]);
    }

    return unique_ptr<RequestMessage>(
        new RequestMessage{protocol, first_message, second_message});
}

const vector<uint8_t> ResponseMessage::write() const
{
    vector<uint8_t> response;

    // put the know nothing protocol version at the first byte
    // byte 0
    response.push_back(kn_protocol);

    // Store the number of messages
    response.push_back(2);

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
    for (const auto &b : value_size)
    {
        response.push_back(b);
    }

    // byte 10 - N
    // append the value
    for (const auto &ele : value)
    {
        response.push_back(ele);
    }

    return response;
}
