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
    bool exists(const std::string key) const;
    void copy_to(const std::string key, std::int32_t output_fd) const;

  private:
    std::vector<std::unique_ptr<File>> opened_files;
    std::unordered_map<std::string, FileStorage> stored;
    const std::uint8_t success_header[7];
};

class Handler
{
  private:
    Database database;

  public:
    void parse_request(ClientSocket& socket);
};

};  // namespace easykey