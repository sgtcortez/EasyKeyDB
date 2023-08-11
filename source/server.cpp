#include "server.hpp"

#include <algorithm>
#include <arpa/inet.h>
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

using namespace std;
using namespace easykey;

/**
 * TODO: This should be defined by our user. And MUST not be hardcoded
 */
static EpollHandler epoll(5000);

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
    epoll.add(socket.file_descriptor, EPOLLIN);

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
        const auto events = epoll.wait_for_events();
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

            if (event.data.fd == socket.file_descriptor)
            {
                handle_new_connection();
            }
            else if (event.events & EPOLLRDHUP)
            {
                handle_client_disconnected(event.data.fd);
            }
            else if (event.events & EPOLLIN)
            {
                handle_request(current_connections.at(event.data.fd).get());
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
    epoll.del(socket.file_descriptor);
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

    /**
     * Register the accepted socket to those epoll events
     * READ | Edge Triggered | WRITE | PEER SHUTDOW
     */
    epoll.add(client->file_descriptor,
              EPOLLIN | EPOLLET | EPOLLOUT | EPOLLRDHUP);

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

    // Reads the content from the buffer
    client->read();

    const auto client_response = receive_message_callback(*client);
    if (!client_response.empty())
    {
        client->write(client_response);
        cout << "Written data to client !" << endl;
    }
}

void Server::handle_client_disconnected(const std::int32_t file_descriptor)
{
    if (client_disconnected_callback)
    {
        const auto client = current_connections[file_descriptor].get();
        client_disconnected_callback(*client);
    }

    epoll.del(file_descriptor);
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

EpollHandler::EpollHandler(const uint16_t timeout) : timeout(timeout)
{
    file_descriptor = epoll_create1(0);
    if (file_descriptor < 0)
    {
        throw "Epoll Create!";
    }
}

EpollHandler::~EpollHandler()
{
    close(file_descriptor);
}

bool EpollHandler::add(int32_t file_descriptor, uint32_t events) const
{
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.events = events;
    event.data.fd = file_descriptor;
    return epoll_ctl(this->file_descriptor,
                     EPOLL_CTL_ADD,
                     file_descriptor,
                     &event);
}

bool EpollHandler::del(int32_t file_descriptor) const
{
    return epoll_ctl(this->file_descriptor,
                     EPOLL_CTL_DEL,
                     file_descriptor,
                     nullptr);
}

const vector<struct epoll_event> EpollHandler::wait_for_events() const
{
    struct epoll_event events_buffer[32];
    auto total = epoll_wait(file_descriptor,
                            events_buffer,
                            sizeof(events_buffer) / sizeof(events_buffer[0]),
                            timeout);
    if (total < 0)
    {
        cerr << "Epoll wait error!" << endl;
        return {};
    }
    else if (total == 0)
    {
        cout << "Epoll Timeout!" << endl;
        return {};
    }

    vector<struct epoll_event> events;
    events.reserve(total);
    for (auto index = 0; index < total; index++)
    {
        events.push_back(events_buffer[index]);
    }
    return events;
}
