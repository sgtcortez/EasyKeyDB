#pragma once

#include "socket.hpp"

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

};  // namespace easykey