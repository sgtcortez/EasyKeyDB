#include "server.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <vector>

using namespace std;
using namespace easykey;

/**
 * Builds a server socket.
 * This is necessary, because the socket variable is const,
 * so it needs to be build before object construct time
 */
ServerSocket build_server_socket(uint16_t port);

static EpollHandler epoll(500);

Socket::Socket(const int32_t file_descriptor, const struct sockaddr_in address)
    : file_descriptor(file_descriptor), address(address)
{
}

Socket::~Socket()
{
    epoll.del(file_descriptor);
    close(file_descriptor);
}

ServerSocket::ServerSocket(const int32_t file_descriptor,
                           const struct sockaddr_in address)
    : Socket(file_descriptor, address)
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

const ClientSocket *ServerSocket::accept_connection() const
{
    struct sockaddr_in client_address;
    socklen_t size = sizeof(struct sockaddr_in);

    int32_t client_file_descriptor =
        accept(file_descriptor, (struct sockaddr *)&client_address, &size);
    if (client_file_descriptor < 0)
    {
        throw "Could not accept a connection!";
    }
    return new ClientSocket(client_file_descriptor, client_address);
    ;
}

ClientSocket::ClientSocket(const int32_t file_descriptor,
                           const struct sockaddr_in address)
    : Socket(file_descriptor, address)
{
    // available to READ | Requests edge-triggered notification | available
    // WRITE | peer shutdow edge-triggered notification |
    epoll.add(file_descriptor, EPOLLIN | EPOLLET | EPOLLOUT | EPOLLRDHUP);
}
void ClientSocket::send(const vector<uint8_t> message) const
{
    /**
     * Use the write systemcall to write to the client socket
     * man 2 write
     */
    if (write(file_descriptor, message.data(), message.size()) < 0)
    {
        throw "Could not write data!";
    }
}

vector<uint8_t> ClientSocket::get() const
{
    // 16 KiB buffer
    uint8_t buffer[1024 * 16];

    const auto buffer_size = sizeof(buffer) / sizeof(buffer[0]);

    vector<uint8_t> vector;
    do
    {
        // Uses the read system call to read from kernel buffer to our buffer
        // man 2 read
        const auto bytes_read = read(file_descriptor, buffer, buffer_size);
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
               Dispatcher &dispatcher)
    : port(port),
      pending_connections(pending_connections),
      socket(build_server_socket(port)),
      dispatcher(dispatcher)
{
}

void Server::start()
{
    socket.assign_address();
    socket.set_available(pending_connections);

    cout << "Server in mode to accept connections ..." << endl;
    cout << "Server running in port: " << to_string(port) << " and can queue "
         << to_string(pending_connections) << " connections" << endl;
    cout << "Using dispatcher: \"" << dispatcher.name()
         << "\" to dispatch client requests!" << endl;

    running = true;
    do
    {
        for (const auto &event : epoll.wait_for_events())
        {
            if (!running)
            {
                // while waiting for events, or processing some event, the
                // server requested to stop
                break;
            }

            if (event.data.fd == socket.file_descriptor)
            {
                cout << "A new client has arrived!" << endl;
                handle_new_connection();
            }
            else if (event.events & EPOLLRDHUP)
            {
                cout << "A client disconnected!" << endl;
                handle_client_disconnected(event.data.fd);
            }
            else if (event.events & EPOLLIN)
            {
                cout << "A client has sent some data!" << endl;
                handle_request(current_connections.at(event.data.fd).get());
            }
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
    unique_ptr<const ClientSocket> client(socket.accept_connection());
    current_connections.insert(
        make_pair(client->file_descriptor, move(client)));
}

void Server::handle_request(const ClientSocket *client) const
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
    const auto reply = dispatcher.exchange(client->get());
    client->send(reply);
}

void Server::handle_client_disconnected(const std::int32_t file_descriptor)
{
    current_connections.erase(file_descriptor);
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