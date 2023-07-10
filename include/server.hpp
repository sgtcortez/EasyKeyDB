#pragma once

#include <bits/stdint-uintn.h>
#include <cstdint>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <memory>
#include <vector>

namespace easykey
{
/**
 * Forward declarations
 */
class ServerSocket;
class ClientSocket;

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
  public:
    Socket(const std::int32_t file_descriptor,
           const struct sockaddr_in address);
    ~Socket();

    // TOOD: THis must not be public!
    const std::int32_t file_descriptor;

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
    const struct sockaddr_in address;
};

class ServerSocket : public Socket
{
  public:
    ServerSocket(const std::int32_t file_descriptor,
                 const struct sockaddr_in address);
    void assign_address() const;
    void set_available(std::uint16_t backlog_queue) const;
    const ClientSocket* accept_connection() const;
};

class ClientSocket : public Socket
{
  public:
    ClientSocket(const std::int32_t file_descriptor,
                 const struct sockaddr_in address);
    void send(std::vector<uint8_t> message) const;
    std::vector<uint8_t> get() const;
};

/**
 * This class is used to exchange data between client & server
 * The client connects, sends a message, than the server replies.
 * The main basic of client-server architeture.
 */
struct Dispatcher
{
  public:
    virtual std::string name() const = 0;
    virtual std::vector<std::uint8_t> exchange(
        std::vector<std::uint8_t> client_request) = 0;
};

class Server
{
  public:
    Server(const std::uint16_t port,
           const std::uint16_t pending_connections,
           Dispatcher& dispatcher);
    void start();
    void stop();

  private:
    const std::uint16_t port;
    const std::uint16_t pending_connections;
    const ServerSocket socket;
    Dispatcher& dispatcher;

    // A SIGTERM/SIGINT signal set this to false
    bool running;

    /**
     A new connection arrived
    */
    void handle_new_connection();

    /**
     * Check for idle connections and then remove them for current_connectins
     * map
     */
    void check_idle_connections();

    /**
     * Handle client disconnected
     */
    void handle_client_disconnected(std::int32_t file_descriptor);

    /**
     * Handle the read request
     */
    void handle_request(const ClientSocket* client) const;

    /**
     Keep track of the current client connections
    */
    std::unordered_map<std::int32_t, std::unique_ptr<const ClientSocket>>
        current_connections;
};

};  // namespace easykey