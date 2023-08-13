#include "easykey.hpp"
#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <memory>
#include <regex>
#include <string>
#include <vector>
#include "byte_buffer.hpp"
#include "socket.hpp"

using namespace easykey;
using namespace std;
using namespace knownothing;

static const string regex_value = "[^a-zA-Z0-9_]+";
static const regex invalid_key_regex(regex_value);

/**
 * The file must be opened with Read & Write
 * Must be created if does not exist
 * Must be truncated if exists
 */
constexpr static int32_t OPEN_FILE_FLAGS = O_RDWR | O_CREAT | O_TRUNC;

/**
 * The mode(permissions) of the file are:
 * Owner of the file has READ and Write permissions
 * Everyone else does not have any permissions
 */
constexpr static int32_t OPEN_FILE_MODE = S_IRUSR | S_IWUSR;

bool is_key_valid(const string& key);
uint8_t hash(const string key, uint8_t mod);
vector<uint8_t> write_dynamic_content(ResponseStatus status,
                                      const string error_description);

File::File(const int32_t fd, const string filename) : fd(fd), filename(filename)
{
    cout << "Opened file: " << filename
         << " with file descriptor: " << to_string(fd) << endl;
}

File::~File()
{
    cout << "Calling close to descriptor: " << to_string(fd) << endl;
    if (close(fd) < 0)
    {
        cerr << "Could not close the file properly ..." << endl;
    }
}

Database::Database()
{
    uint8_t files = NUMBER_OF_FILES;
    opened_files.reserve(files);
    for (uint8_t index = 0; index < files; index++)
    {
        const auto location = "/tmp/easykey-" + to_string(index) + ".db";
        int32_t fd = open(location.c_str(), OPEN_FILE_FLAGS, OPEN_FILE_MODE);
        if (fd == -1)
        {
            throw "Could not open file: " + location;
        }
        opened_files.push_back(unique_ptr<File>(new File(fd, location)));
    }
}

void Database::write(const string key, const vector<uint8_t> data)
{
    auto file = this->opened_files[::hash(key, NUMBER_OF_FILES)].get();

    // The file offset is set to the size of the file plus offset bytes.
    const auto current_file_size = lseek(file->fd, 0, SEEK_END);

    vector<uint8_t> stored_data;
    stored_data.reserve(data.size() + sizeof(uint32_t));

    // serialize the 4 integer of the user data size
    stored_data.push_back(data.size());
    stored_data.push_back(data.size() >> 8);
    stored_data.push_back(data.size() >> 16);
    stored_data.push_back(data.size() >> 24);

    copy(data.begin(), data.end(), back_inserter(stored_data));

    // Use write system call to append data at the end of the file
    if (::write(file->fd, stored_data.data(), stored_data.size()) == -1)
    {
        perror("write: ");
    }

    cout << "Write content of the key: " << key
         << " at file: " << file->filename << endl;

    // Constructs the storage
    FileStorage storage{stored_data.size(), current_file_size, file};

    // Add to our database
    stored.insert(make_pair(key, storage));
}

const FileStorage* Database::read(const string key) const
{
    const auto value = stored.find(key);
    if (value == stored.end())
    {
        return nullptr;
    }
    return &(value->second);
}

void Handler::parse_request(ClientSocket& socket)
{
    try
    {
        const auto protocol = static_cast<knownothing::Protocol>(
            socket.read_buffer.get_integer1());
        if (protocol != knownothing::Protocol::V1)
        {
            cerr << "Invalid Know Nothing Protocol " << endl;
            const auto response = write_dynamic_content(
                ResponseStatus::CLIENT_ERROR,
                "Only the first version of Know Nothing is supported!");
            socket.write(response.data(), response.size(), false);
            return;
        }

        const auto messages = socket.read_buffer.get_integer1();
        if (messages != 1 && messages != 2)
        {
            // handle
            cerr << "Invalid Easy Key Message!"
                 << " One or Two messages are allowed. Provided is " << messages
                 << endl;
            const auto response = write_dynamic_content(
                ResponseStatus::CLIENT_ERROR,
                "Invalid EasyKey Message! Allowed is 1 or 2 Messages!");
            socket.write(response.data(), response.size(), false);
            return;
        }

        const auto first_message_size = socket.read_buffer.get_integer4();
        string first_message;
        first_message.reserve(first_message_size);

        // Return the next buffer messages
        for (const auto& ch : socket.read_buffer.get_next(first_message_size))
        {
            first_message += ch;
        }

        if (!is_key_valid(first_message))
        {
            cerr << "The key: " << first_message << " is not valid ..." << endl;
            const auto response = write_dynamic_content(
                ResponseStatus::CLIENT_ERROR,
                "The key: " + first_message +
                    " is not valid! Must be alphanumeric!");
            socket.write(response.data(), response.size(), false);
            return;
        }

        // Its a read operation
        if (messages == 1)
        {
            const auto value = database.read(first_message);
            if (value == nullptr)
            {
                // not found
                cerr << "The key: " << first_message << " was not found!"
                     << endl;
                const auto response =
                    write_dynamic_content(ResponseStatus::CLIENT_ERROR,
                                          "The key" + first_message +
                                              " was not found!");
                socket.write(response.data(), response.size(), false);
            }
            // send the first message
            socket.write(success_header,
                         sizeof(success_header) / sizeof(success_header[0]),
                         true);
            // send the second message
            socket.write(value->file->fd, value->offset, value->size);
            return;
        }

        // Its a write operation
        const auto second_message_size = socket.read_buffer.get_integer4();
        const auto second_message =
            socket.read_buffer.get_next(second_message_size);

        database.write(first_message, second_message);
        const auto response =
            write_dynamic_content(ResponseStatus::OK,
                                  "The key: " + first_message +
                                      " was successfully written!");
        socket.write(response.data(), response.size(), false);
        cout << "The key: " << first_message << " was successfully written!"
             << endl;
    }
    catch (const EmptyBufferException& exception)
    {
        cerr << "Message does not follow KnowNothing Protocol specification!"
             << endl;
        const auto response = write_dynamic_content(
            ResponseStatus::CLIENT_ERROR,
            "Message does not follow KnowNothing Protocol Specification!");
        socket.write(response.data(), response.size(), false);
        return;
    }
}

bool is_key_valid(const string& key)
{
    return !key.empty() && !regex_search(key, invalid_key_regex);
}

uint8_t hash(const string key, uint8_t mod)
{
    return key.size() % mod;
}

vector<uint8_t> write_dynamic_content(ResponseStatus status,
                                      const string error_description)
{
    vector<uint8_t> response;
    response.reserve(100);

    // Know Nothing Protocol Version
    response.push_back(Protocol::V1);

    // Number of messages(always 2)
    response.push_back(2);

    // Status code size, its a 4 byte value
    response.push_back(1);
    response.push_back(0);
    response.push_back(0);
    response.push_back(0);

    // Status code value(2 is client error)
    response.push_back(static_cast<uint8_t>(status));

    // Serialize the message size
    response.push_back(error_description.size());
    response.push_back(error_description.size() >> 8);
    response.push_back(error_description.size() >> 16);
    response.push_back(error_description.size() >> 24);

    for (const auto& ch : error_description)
    {
        response.push_back(ch);
    }
    return response;
}
