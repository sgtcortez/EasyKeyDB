#include "socket.hpp"

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <iostream>
#include <netinet/in.h>

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

template <>
Socket::Option<int32_t> const Socket::Option<int32_t>::TCP_NO_DELAY(
    IPPROTO_TCP,
    TCP_NODELAY);

template <>
Socket::Option<struct linger> const Socket::Option<struct linger>::LINGER(
    SOL_SOCKET,
    SO_LINGER);

};  // namespace easykey

Socket::Socket(const int32_t file_descriptor) : file_descriptor(file_descriptor)
{
}

Socket::~Socket()
{
    close(file_descriptor);
}

ServerSocket::ServerSocket(const int32_t file_descriptor,
                           const struct sockaddr_in address)
    : Socket(file_descriptor), address(address)
{
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

ServerSocket ServerSocket::from(uint16_t port)
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

    return ServerSocket(file_descriptor, address);
}
