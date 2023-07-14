#pragma once

#include <bits/stdint-uintn.h>
#include <cstdint>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <memory>
#include <vector>
#include <chrono>
#include <functional>
#include <string>

namespace easykey
{
/**
 * Forward declarations
 */
class ServerSocket;
class ClientSocket;
class Server;

// https://en.cppreference.com/w/cpp/chrono
// Is there a better way?
using timestamp = std::chrono::time_point<
    std::chrono::steady_clock,
    std::chrono::duration<std::uint64_t, std::ratio<1, 1000000000>>>;

/**
 * This was inspired from:
 * https://github.com/an-tao/trantor/blob/35592d542f8eeb1345844cfa8a202ed96707d379/trantor/net/callbacks.h#L34
 */
using ReceiveMessageCallback =
    std::function<std::vector<uint8_t>(const ClientSocket&,
                                       std::vector<std::uint8_t>)>;

class EpollHandler
{
  public:
    EpollHandler(const std::uint16_t timeout);
    ~EpollHandler();
    bool add(std::int32_t file_descriptor, std::uint32_t events) const;
    bool del(std::int32_t file_descriptor) const;
    const std::vector<struct epoll_event> wait_for_events() const;

  private:
    std::uint32_t file_descriptor;
    std::uint16_t timeout;
};

class Socket
{
    friend Server;

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
      private:
        Option(const std::int32_t level, const std::int32_t value);

      public:
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
    };

    template <typename VALUE_TYPE>
    struct OptionValue
    {
      public:
        OptionValue(const VALUE_TYPE value_type,
                    const Option<VALUE_TYPE>& type);

        /**
         * Since the options are static instances, we can use lvalue here
         */
        const Option<VALUE_TYPE>& type;

        const VALUE_TYPE value_type;
    };

    template <typename TYPE>
    void set_option(const OptionValue<TYPE> option) const;

    template <typename TYPE>
    const OptionValue<TYPE> get_option(const Option<TYPE>& type) const;

  protected:
    const std::int32_t file_descriptor;
};

class ServerSocket : public Socket
{
  public:
    ServerSocket(const std::int32_t file_descriptor, const sockaddr_in address);
    void assign_address() const;
    void set_available(std::uint16_t backlog_queue) const;
    ClientSocket* accept_connection() const;

  private:
    struct sockaddr_in address;
};

class ClientSocket : public Socket
{
    // Server class can access private data
    friend Server;

  public:
    ClientSocket(const std::int32_t file_descriptor,
                 const std::string host_ip,
                 const std::uint16_t port);
    const easykey::timestamp start;
    const std::string host_ip;
    const std::uint16_t port;

    easykey::timestamp last_seen;
    std::uint32_t iterations;

  private:
    /**
     * Reads the content of the socket buffer.
     */
    const std::vector<std::uint8_t> read() const;

    /**
     * Writes the content to the socket buffer
     */
    void write(const std::vector<std::uint8_t> content) const;
};

class Server
{
  public:
    Server(const std::uint16_t port,
           const std::uint16_t pending_connections,
           const ReceiveMessageCallback receive_message_callback);
    void start();
    void stop();

  private:
    const std::uint16_t port;
    const std::uint16_t pending_connections;
    const ServerSocket socket;
    const ReceiveMessageCallback receive_message_callback;

    // A SIGTERM/SIGINT signal set this to false
    bool running;

    /**
     A new connection arrived
    */
    void handle_new_connection();

    /**
     * Check for idle connections and then remove them for current_connectins
     * map.
     * This method must run every time that epoll returns timeout(exit = 0) and,
     * must run periodically otherwise, we can have a false positively(if some
     * connections are always sending data)
     */
    void check_idle_connections();

    /**
     * Handle client disconnected
     */
    void handle_client_disconnected(std::int32_t file_descriptor);

    /**
     * Handle the read request
     */
    void handle_request(ClientSocket* client);

    /**
     Keep track of the current client connections
    */
    std::unordered_map<std::int32_t, std::unique_ptr<ClientSocket>>
        current_connections;
};

};  // namespace easykey