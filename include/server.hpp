#pragma once

#include <bits/stdint-uintn.h>
#include <cstdint>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdlib>
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

using ClientConnectedCallback = std::function<void(const ClientSocket&)>;
using ClientDisconnectedCallback = std::function<void(const ClientSocket&)>;

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
      Option(const std::int32_t level, const std::int32_t value) : level(level), value(value), size(sizeof(OPTION_TYPE)) {}

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

       * https://en.wikipedia.org/wiki/Nagle%27s_algorithm its useful to undestand this ...
      */
      static const Option<std::int32_t> TCP_NO_DELAY;

    };

    template <typename VALUE_TYPE>
    struct OptionValue
    {

      OptionValue(const VALUE_TYPE value_type, const Option<VALUE_TYPE>& type):value_type(value_type), type(type){}

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
      void *opt_value = &(t);
      socklen_t opt_len = option.type.size;
      if (::setsockopt(file_descriptor, level, opt_name, opt_value, opt_len) == -1)
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
       * We can achive this, with templates, so its know at compile time how big it is.
      */
      std::int8_t buffer[type.size];
      socklen_t opt_len;
      if (::getsockopt(socketfd, level, opt_name, buffer, &opt_len) == -1)
      {
          perror("getsockopt");
      }
      TYPE *t = reinterpret_cast<TYPE *>(buffer);
      const OptionValue<TYPE> result(*t, type);
      return result;      
    }

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
           const ReceiveMessageCallback receive_message_callback,
           const ClientConnectedCallback client_connected_callback,
           const ClientDisconnectedCallback client_disconnected_callback);

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
    const ClientConnectedCallback client_connected_callback;
    const ClientDisconnectedCallback client_disconnected_callback;

    // A SIGTERM/SIGINT signal set this to false
    bool running;

    /**
     A new connection arrived
     Calls client_connected_callback if any
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
     * calls client_disconnected_callback if any
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

template <>
Socket::Option<int32_t> const Socket::Option<int32_t>::TCP_CORKING;

template <>
Socket::Option<int32_t> const Socket::Option<int32_t>::MINIMUM_BYTES_TO_CONSIDER_BUFFER_AS_READABLE;

template <>
Socket::Option<int32_t> const Socket::Option<int32_t>::READ_BUFFER_SIZE_TYPE;

template <>
Socket::Option<struct timeval> const Socket::Option<struct timeval>::READ_TIMEOUT;

template<>
Socket::Option<std::int32_t> const Socket::Option<std::int32_t>::REUSE_ADDRESS;

template<>
Socket::Option<std::int32_t> const Socket::Option<std::int32_t>::REUSE_PORT;

template <>
Socket::Option<std::int32_t> const Socket::Option<std::int32_t>::TCP_NO_DELAY;

};  // namespace easykey