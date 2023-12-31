#include "server.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <bits/types/struct_timeval.h>
#include <netinet/tcp.h>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <memory>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>
#include "socket.hpp"
#include "io_notifier.hpp"

using namespace std;
using namespace easykey;

Server::Server(const uint16_t port,
               const uint16_t pending_connections,
               const ReceiveMessageCallback receive_message_callback,
               const ClientConnectedCallback client_connected_callback,
               const ClientDisconnectedCallback client_disconnected_callback)
    : port(port),
      pending_connections(pending_connections),
      socket(ServerSocket::from(port)),
      receive_message_callback(receive_message_callback),
      client_connected_callback(client_connected_callback),
      client_disconnected_callback(client_disconnected_callback)
{
}

Server::Server(const uint16_t port,
               const uint16_t pending_connections,
               const ReceiveMessageCallback receive_message_callback)
    : port(port),
      pending_connections(pending_connections),
      socket(ServerSocket::from(port)),
      receive_message_callback(receive_message_callback),
      client_connected_callback(nullptr),
      client_disconnected_callback(nullptr)
{
}

void Server::start()
{
    /**
     * Register the server socket as this event in epoll
     */
    io_notifier.add_event(Event(socket.file_descriptor).add(EventType::READ));

    socket.set_option(Socket::OptionValue<std::int32_t>{
        1, Socket::Option<int32_t>::REUSE_ADDRESS});
    socket.set_option(
        Socket::OptionValue<std::int32_t>{1,
                                          Socket::Option<int32_t>::REUSE_PORT});

    socket.assign_address();
    socket.set_available(pending_connections);

    cout << "Server in mode to accept connections ..." << endl;
    cout << "Server running in port: " << to_string(port) << " and can queue "
         << to_string(pending_connections) << " connections" << endl;

    running = true;
    uint16_t iterations = 0;
    do
    {
        const auto events =
            io_notifier.wait_for_events(std::chrono::seconds(10));
        if (events.empty())
        {
            check_idle_connections();
        }
        for (const auto &event : events)
        {
            if (!running)
            {
                // while waiting for events, or processing some event, the
                // server requested to stop
                break;
            }
            if (event.is_for(socket.file_descriptor))
            {
                handle_new_connection();
            }
            else if (event.has(EventType::CLOSE_CONNECTION))
            {
                handle_client_disconnected(event.file_descriptor);
            }
            else if (event.has(EventType::READ))
            {
                handle_request(
                    current_connections.at(event.file_descriptor).get());
            }
        }
        iterations++;

        /**
         * Every 1000 iterations, we check for idle connections
         */
        if (iterations == 1000)
        {
            check_idle_connections();
            iterations = 0;
        }

    } while (running);

    cout << "The server has stopped!" << endl;

    /*
     * Removes the server socket from the epoll handler before closing epoll
     * file descriptor
     */
    io_notifier.delete_event(Event(socket.file_descriptor));
}

void Server::stop()
{
    running = false;
    cout << "Request to stop the server accepted! Gracefully stopping the "
            "server!"
         << endl;
}

void Server::handle_new_connection()
{
    unique_ptr<ClientSocket> client(socket.accept_connection());

    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    // 300 milliseconds of read block timeout if needed ...
    // This is needed, because the client can sends tons of KiB at one request,
    // and the kernel might not be able to return everything at just once read
    // system call
    read_timeout.tv_usec = 1000 * 300;

    client->set_option(Socket::OptionValue<struct timeval>(
        read_timeout, Socket::Option<struct timeval>::READ_TIMEOUT));

    /**
     * Register the accepted socket to those epoll events
     * READ | Edge Triggered | WRITE | PEER SHUTDOW
     */
    io_notifier.add_event(Event(client->file_descriptor)
                              .add(EventType::READ)
                              .add(EventType::CLOSE_CONNECTION)
                              .add(EventType::WRITE));

    if (client_connected_callback)
    {
        client_connected_callback(*client);
    }
    current_connections.insert(
        make_pair(client->file_descriptor, move(client)));
}

void Server::handle_request(ClientSocket *client)
{
    /**
     TODO: This will not work in the real world ...
     IF the client produce faster than the consumer, the kernel will store the
     messages at the same buffer then, for example, if the client sends
     write(server_fd, buffer1, buffer1_size) and write(server_fd, buffer2,
     buffer2_size) when we read, we read these two messages as just one message
     ... So, there is a need to share a buffer. For example, we could have the
     same abstraction of InputStream and OutputStream from Java
    */

    /**
     * Updates the last seen time
     */
    client->last_seen = chrono::steady_clock::now();

    /**
     * Increment the number of iterations of the client with the server
     */
    client->iterations++;

    // Send the message to the client
    receive_message_callback(*client);
}

void Server::handle_client_disconnected(const std::int32_t file_descriptor)
{
    if (client_disconnected_callback)
    {
        const auto client = current_connections[file_descriptor].get();
        client_disconnected_callback(*client);
    }

    io_notifier.delete_event(Event(file_descriptor));
    current_connections.erase(file_descriptor);
}

void Server::check_idle_connections()
{
    cout << "Checking for idle connections ..." << endl;
    const auto now = chrono::steady_clock::now();
    vector<int32_t> to_remove;
    for (const auto &connection : current_connections)
    {
        const auto difference = chrono::duration_cast<chrono::seconds>(
                                    now - connection.second->last_seen)
                                    .count();
        // 30 Seconds of idle is allowed
        if (difference >= 300)
        {
            // Can remove this connection
            to_remove.push_back(connection.first);
        }
    }
    for (const auto &fd : to_remove)
    {
        cout << "Removing connection: " << to_string(fd)
             << " due to inactively!" << endl;
        handle_client_disconnected(fd);
    }
}
