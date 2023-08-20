#pragma once

#include "byte_buffer.hpp"

#include <cstdint>  // For fixed-width integer types like std::int32_t, std::uint16_t, etc.
#include <cstdlib>       // For functions like perror
#include <cstring>       // For functions like memcpy
#include <netinet/in.h>  // For the sockaddr_in structure
#include <sys/socket.h>  // For socket-related functions and constants
#include <sys/time.h>    // For struct timeval
#include <sys/types.h>
#include <unistd.h>  // For close function
#include <functional>
#include <memory>
#include <vector>  // For std::vector
#include <string>  // For std::string
#include <chrono>  // For std::chrnono

namespace easykey
{
// https://en.cppreference.com/w/cpp/chrono
// Is there a better way?
using timestamp = std::chrono::time_point<
    std::chrono::steady_clock,
    std::chrono::duration<std::uint64_t, std::ratio<1, 1000000000>>>;

/**
 * Forward declarations
 */
class ServerSocket;
class ClientSocket;
class Server;

class Socket
{
    friend easykey::Server;

  public:
    Socket(const std::int32_t file_descriptor);
    ~Socket();

    /**
     * The socket options.
     * This class is used to retrieve information from the user, without the
     * need of him to specify it
     */
    template <typename OPTION_TYPE>
    struct Option
    {
        Option(const std::int32_t level, const std::int32_t value)
            : level(level), value(value), size(sizeof(OPTION_TYPE))
        {
        }

        /**
         * The level argument
         */
        const std::int32_t level;

        /**
        * The optname argument
        )*/
        const std::int32_t value;

        /**
         * The optlen argument
         */
        const std::int32_t size;

        /**
         * read from socket buffer size
         */
        static const Option<std::int32_t> READ_BUFFER_SIZE_TYPE;

        /**
         * When reading from socket, we define the timeout
         */
        static const Option<struct timeval> READ_TIMEOUT;

        /**
         * Minimum number of bytes to consider the buffer as readable
         * Can be useful if want to use batch messages
         */
        static const Option<std::int32_t>
            MINIMUM_BYTES_TO_CONSIDER_BUFFER_AS_READABLE;

        /**
         * This option is used to enable the TCP Corking mechanism,
         * which delays sending small packets in order to optimize network
         * throughput.
         */
        static const Option<std::int32_t> TCP_CORKING;

        /**
         * Indicates that the rules used in validating addresses
         * supplied in a bind(2) call should allow reuse of local
         * addresses. For AF_INET sockets this means that a socket
         * may bind, except when there is an active listening socket
         * bound to the address. When the listening socket is bound
         * to INADDR_ANY with a specific port then it is not possible
         * to bind to this port for any local address.
         */
        static const Option<std::int32_t> REUSE_ADDRESS;

        /**
         * Permits multiple AF_INET or AF_INET6 sockets to be bound
         * to an identical socket address. This option must be set
         * on each socket (including the first socket) prior to
         * calling bind(2) on the socket. To prevent port hijacking,
         * all of the processes binding to the same address must have
         * the same effective UID. This option can be employed with
         * both TCP and UDP sockets.
         */
        static const Option<std::int32_t> REUSE_PORT;
        /*
         *  If set, disable the Nagle algorithm.  This means that
         *  segments are always sent as soon as possible, even if
         *  there is only a small amount of data.  When not set, data
         *  is buffered until there is a sufficient amount to send
         *  out, thereby avoiding the frequent sending of small
         *  packets, which results in poor utilization of the network.

         * https://en.wikipedia.org/wiki/Nagle%27s_algorithm its useful to
         undestand this ...
        */
        static const Option<std::int32_t> TCP_NO_DELAY;

        /**
         * If defined, tries to send al enqued messages before socket.close or
         * socket.shutdown return. So, if the send exceeds the timeout defined
         * in the linger option
         */
        static const Option<struct linger> LINGER;
    };

    template <typename VALUE_TYPE>
    struct OptionValue
    {
        OptionValue(const VALUE_TYPE value_type, const Option<VALUE_TYPE>& type)
            : value_type(value_type), type(type)
        {
        }

        /**
         * Since the options are static instances, we can use lvalue here
         */
        const Option<VALUE_TYPE>& type;

        const VALUE_TYPE value_type;
    };

    template <typename TYPE>
    void set_option(const OptionValue<TYPE> option) const
    {
        int32_t level = option.type.level;
        int32_t opt_name = option.type.value;
        TYPE t = option.value_type;
        void* opt_value = &(t);
        socklen_t opt_len = option.type.size;
        if (::setsockopt(
                file_descriptor, level, opt_name, opt_value, opt_len) == -1)
        {
            perror("setsockopt");
        }
    };

    template <typename TYPE>
    const OptionValue<TYPE> get_option(const Option<TYPE>& type) const
    {
        int32_t socketfd = file_descriptor;
        int32_t level = type.level;
        int32_t opt_name = type.value;

        /**
         * NOTE: This is not a Variable Length Array.
         * We can achive this, with templates, so its know at compile time how
         * big it is.
         */
        std::int8_t buffer[type.size];
        socklen_t opt_len;
        if (::getsockopt(socketfd, level, opt_name, buffer, &opt_len) == -1)
        {
            perror("getsockopt");
        }
        TYPE* t = reinterpret_cast<TYPE*>(buffer);
        const OptionValue<TYPE> result(*t, type);
        return result;
    }

  protected:
    const std::int32_t file_descriptor;
};

class ServerSocket : public Socket
{
  private:
    ServerSocket(const std::int32_t file_descriptor, const sockaddr_in address);

  public:
    static ServerSocket from(const std::uint16_t port);
    void assign_address() const;
    void set_available(std::uint16_t backlog_queue) const;
    ClientSocket* accept_connection() const;

  private:
    struct sockaddr_in address;
};

class ClientSocket : public Socket
{
    friend easykey::Server;

  public:
    ClientSocket(const std::int32_t file_descriptor,
                 const std::string host_ip,
                 const std::uint16_t port);

    const easykey::timestamp start;
    const std::string host_ip;
    const std::uint16_t port;

    easykey::timestamp last_seen;
    std::uint32_t iterations;

  public:
    /**
     * This buffer is where the data is stored after every socket read
     */
    ByteBuffer read_buffer;

    /**
     * Writes the content to the socket buffer
     * If more_coming is true, we tell the kernel to add to the socket buffer,
     * instead of writing immediately
     */
    void write(const std::uint8_t* buffer,
               const std::uint32_t size,
               bool more_coming) const;

    /**
     * Writes the content of the file descriptor, starting from offset and with
     * the size of size
     */
    void write(const std::int32_t file_descriptor,
               off_t offset,
               const std::int64_t size) const;
};

template <>
Socket::Option<int32_t> const Socket::Option<int32_t>::TCP_CORKING;

template <>
Socket::Option<int32_t> const
    Socket::Option<int32_t>::MINIMUM_BYTES_TO_CONSIDER_BUFFER_AS_READABLE;

template <>
Socket::Option<int32_t> const Socket::Option<int32_t>::READ_BUFFER_SIZE_TYPE;

template <>
Socket::Option<struct timeval> const
    Socket::Option<struct timeval>::READ_TIMEOUT;

template <>
Socket::Option<std::int32_t> const Socket::Option<std::int32_t>::REUSE_ADDRESS;

template <>
Socket::Option<std::int32_t> const Socket::Option<std::int32_t>::REUSE_PORT;

template <>
Socket::Option<std::int32_t> const Socket::Option<std::int32_t>::TCP_NO_DELAY;

template <>
Socket::Option<struct linger> const Socket::Option<struct linger>::LINGER;

};  // namespace easykey