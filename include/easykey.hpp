#pragma once

#include <sys/types.h>
#include "byte_buffer.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "socket.hpp"

#define NUMBER_OF_FILES 5

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
enum class ResponseStatus : std::uint8_t
{
    OK = 0x01,
    CLIENT_ERROR = 0x02,
    SERVER_ERROR = 0x03,
};

struct File
{
    /**
     * The file descriptor
     */
    const std::int32_t fd;

    /**
     * The file name
     */
    const std::string filename;

    File(const std::int32_t fd, const std::string filename);
    ~File();

    /**
     * https://en.cppreference.com/w/cpp/language/rule_of_three
     *
     */
    File(const File&) = delete;
    File(File&&) = delete;
    File operator=(const File&) = delete;
    File operator=(File&&) = delete;
};

struct FileStorage
{
    /**
     * The value stored size
     */
    const std::uint64_t size;

    /**
     * Where the value starts
     */
    const off_t offset;

    /**
     * In which file is it located
     */
    const File* file;
};

class Database
{
  public:
    Database();
    void write(const std::string key, const std::vector<std::uint8_t> data);
    const FileStorage* read(const std::string key) const;

  private:
    std::vector<std::unique_ptr<File>> opened_files;
    std::unordered_map<std::string, FileStorage> stored;
};

class Handler
{
  private:
    Database database;
    const std::uint8_t success_header[7] = {
        0x01,  // know nothing protocol
        0x02,  // number of messages
        0x01,  // first message byte
        0x00,  // second message byte
        0x00,  // third message byte
        0x00,  // fourth message byte
        0x01   // First message value(1 sucess)
    };

  public:
    void parse_request(ClientSocket& socket);
};

};  // namespace easykey