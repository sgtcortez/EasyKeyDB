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
#include <asm-generic/socket.h>
#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace easykey;

namespace easykey
{
template <>
Socket::Option<int32_t> const Socket::Option<int32_t>::READ_BUFFER_SIZE_TYPE(
    SOL_SOCKET,
    SO_RCVBUF);

template <>
Socket::Option<struct timeval> const Socket::Option<
    struct timeval>::READ_TIMEOUT(SOL_SOCKET, SO_RCVTIMEO);

template <>
Socket::Option<int32_t> const Socket::Option<
    int32_t>::MINIMUM_BYTES_TO_CONSIDER_BUFFER_AS_READABLE(SOL_SOCKET,
                                                           SO_RCVLOWAT);

template <>
Socket::Option<int32_t> const Socket::Option<int32_t>::TCP_CORKING(IPPROTO_TCP,
                                                                   TCP_CORK);

template <>
Socket::Option<int32_t> const Socket::Option<int32_t>::REUSE_ADDRESS(
    SOL_SOCKET,
    SO_REUSEADDR);

template <>
Socket::Option<int32_t> const Socket::Option<int32_t>::REUSE_PORT(SOL_SOCKET,
                                                                  SO_REUSEPORT);

};  // namespace easykey

/**
 * Builds a server socket.
 * This is necessary, because the socket variable is const,
 * so it needs to be build before object construct time
 */
ServerSocket build_server_socket(uint16_t port);

/**
 * TODO: This should be defined by our user. And MUST not be hardcoded
 */
static EpollHandler epoll(5000);

Socket::Socket(const int32_t file_descriptor) : file_descriptor(file_descriptor)
{
}

Socket::~Socket()
{
    epoll.del(file_descriptor);
    close(file_descriptor);
}

ServerSocket::ServerSocket(const int32_t file_descriptor,
                           const struct sockaddr_in address)
    : Socket(file_descriptor), address(address)
{
    epoll.add(file_descriptor, EPOLLIN);
}

void ServerSocket::assign_address() const
{
    if (bind(file_descriptor, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        throw "Could not assign address to the socket!";
    }
}

void ServerSocket::set_available(uint16_t backlog_queue) const
{
    if (listen(file_descriptor, backlog_queue) < 0)
    {
        throw "Could not set socket to passive mode!";
    }
}

ClientSocket *ServerSocket::accept_connection() const
{
    struct sockaddr_in client_address;
    socklen_t size = sizeof(struct sockaddr_in);

    int32_t client_file_descriptor =
        accept(file_descriptor, (struct sockaddr *)&client_address, &size);
    if (client_file_descriptor < 0)
    {
        throw "Could not accept a connection!";
    }

    string host_ip = inet_ntoa(client_address.sin_addr);
    uint16_t port = ntohs(client_address.sin_port);
    return new ClientSocket(client_file_descriptor, host_ip, port);
}

ClientSocket::ClientSocket(const int32_t file_descriptor,
                           const string host_ip,
                           const uint16_t port)
    : Socket(file_descriptor),
      start(chrono::steady_clock::now()),
      host_ip(host_ip),
      port(port)
{
    // available to READ | Requests edge-triggered notification | available
    // WRITE | peer shutdow edge-triggered notification |
    epoll.add(file_descriptor, EPOLLIN | EPOLLET | EPOLLOUT | EPOLLRDHUP);

    // The last seen variable to check for idle connections once in a while
    last_seen = chrono::steady_clock::now();

    // Starts the iterations with zero
    iterations = 0;
}
void ClientSocket::write(const vector<uint8_t> message) const
{
    /*
     * Use the write systemcall to write to the client socket
     * man 2 write
     */
    if (::write(file_descriptor, message.data(), message.size()) < 0)
    {
        throw "Could not write data!";
    }
}

const vector<uint8_t> ClientSocket::read() const
{
    // 16 KiB buffer
    uint8_t buffer[1024 * 16];

    const auto buffer_size = sizeof(buffer) / sizeof(buffer[0]);

    vector<uint8_t> vector;
    do
    {
        // Uses the read system call to read from kernel buffer to our buffer
        // man 2 read
        const auto bytes_read = ::read(file_descriptor, buffer, buffer_size);
        if (bytes_read < 0)
        {
            cerr << "Could not execute read systemcall in fd: "
                 << to_string(file_descriptor) << endl;
            return {0};
        }

        // TODO: Since vector is backed by an array, we need to improve the
        // number of times that realloc is done under the hood to be more
        // effective Maybe(?)
        // https://cplusplus.com/reference/vector/vector/reserve/

        copy(buffer, buffer + bytes_read, back_inserter(vector));

        // We request for N bytes, returned N - 1, that means that we have
        // nothing else to read from the file descriptor
        if (bytes_read < buffer_size)
        {
            break;
        }

    } while (true);
    return vector;
}

Server::Server(const uint16_t port,
               const uint16_t pending_connections,
               const ReceiveMessageCallback receive_message_callback,
               const ClientConnectedCallback client_connected_callback,
               const ClientDisconnectedCallback client_disconnected_callback)
    : port(port),
      pending_connections(pending_connections),
      socket(build_server_socket(port)),
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
      socket(build_server_socket(port)),
      receive_message_callback(receive_message_callback),
      client_connected_callback(nullptr),
      client_disconnected_callback(nullptr)
{
}

void Server::start()
{
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
    const auto content = client->read();
    const auto client_response = receive_message_callback(*client, content);
    if (!client_response.empty())
    {
        client->write(client_response);
    }
}

void Server::handle_client_disconnected(const std::int32_t file_descriptor)
{
    if (client_disconnected_callback)
    {
        const auto client = current_connections[file_descriptor].get();
        client_disconnected_callback(*client);
    }
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
        if (difference >= 30)
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

ServerSocket build_server_socket(uint16_t port)
{
    const auto ipv4 = AF_INET;

    const auto file_descriptor = socket(ipv4, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (file_descriptor < 0)
    {
        throw "Could not open socket!!!";
    }
    sockaddr_in address;
    memset(&address, 0, sizeof(struct sockaddr_in));

    address.sin_family = ipv4;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    return {file_descriptor, address};
}
