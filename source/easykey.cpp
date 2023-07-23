#include "easykey.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include "byte_buffer.hpp"

using namespace easykey;
using namespace std;
using namespace knownothing;

static const string regex_value = "[^a-zA-Z0-9_]+";
static const regex invalid_key_regex(regex_value);

static bool is_key_valid(const string& key)
{
    return !key.empty() && !regex_search(key, invalid_key_regex);
}

unique_ptr<RequestMessage> RequestMessage::read(ByteBuffer& buffer)
{
    /**
     * First byte is the protocol version
     */
    const auto protocol =
        static_cast<knownothing::Protocol>(buffer.get_integer1());

    const auto messages_number = buffer.get_integer1();
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
    const auto first_message_size = buffer.get_integer4();

    /**
     * From the fifth byte until first messge size, its the first message
     */
    const auto teste = buffer.get_next(first_message_size);
    string first_message;
    for (const auto& c : teste)
    {
        char c1 = (char)c;
        first_message += c1;
    }

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
    const auto second_message_size = buffer.get_integer4();

    /**
     * Creates the vector for the second message.
     * NOTE: This is just a raw byte array, so no data is checked!
     */
    vector<uint8_t> second_message = buffer.get_next(second_message_size);

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

    // append the 4 byte value size
    // byte 6 - 9
    response.push_back(value.size());
    response.push_back(value.size() >> 8);
    response.push_back(value.size() >> 16);
    response.push_back(value.size() >> 24);

    // byte 10 - N
    // append the value
    for (const auto& ele : value)
    {
        response.push_back(ele);
    }

    return response;
}
