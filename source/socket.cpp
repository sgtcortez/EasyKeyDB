#include "socket.hpp"

//#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <iostream>
#include <memory>
#include <system_error>
#include <vector>
#include <netinet/in.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>

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

    /*
        if (fcntl(client_file_descriptor, F_SETFL, SOCK_NONBLOCK) < 0)
        {
            const auto error = "Could not set the socket file descriptor " +
                               to_string(client_file_descriptor) +
                               "as non blocking";
            cerr << error << endl;
            throw error;
        }
    */
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
      port(port),
      read_buffer([this]() -> vector<uint8_t> {
          // 1MiB  buffer
          uint8_t buffer[1024 * 1024 * 1];
          const int64_t buffer_size = sizeof(buffer) / sizeof(buffer[0]);

          // Uses the read system call to read from kernel buffer to our buffer
          // man 2 read
          const auto bytes_read =
              ::read(this->file_descriptor, buffer, buffer_size);

          // An error occurred
          if (bytes_read < 0)
          {
              const auto error_code = errno;
              if (error_code == EAGAIN || error_code == EWOULDBLOCK)
              {
                  /**
                   The file descriptor is market as non blocking and the read
                   would block because there are no bytes available or
                   Socket::MINIMUM_BYTES_TO_CONSIDER_BUFFER_AS_READABLE is in
                   use
                  */
                  cout
                      << "No data available in the buffer for file descriptor: "
                      << this->file_descriptor << endl;
                  return {};
              }
              cerr << "Unexpected error occurred while trying to read from "
                      "file descriptor: "
                   << this->file_descriptor << endl;
              return {};
          }
          vector<uint8_t> bytes(buffer, buffer + bytes_read);
          return bytes;
      })
{
    // The last seen variable to check for idle connections once in a while
    last_seen = chrono::steady_clock::now();

    // Starts the iterations with zero
    iterations = 0;
}
void ClientSocket::write(const uint8_t *buffer,
                         const uint32_t size,
                         bool more_coming) const
{
    /**
     * With the MSG_MORE, we tell the kernel that we have more data to send to
     * this socket https://man7.org/linux/man-pages/man2/send.2.html
     */
    int32_t flags = more_coming ? MSG_MORE : 0;
    if (::send(file_descriptor, buffer, size, flags) == -1)
    {
        cerr << "Could not send data to the filedescriptor: " << file_descriptor
             << endl;
    }
}

void ClientSocket::write(const int32_t file_descriptor,
                         off_t offset,
                         const int64_t size) const
{
    if (::sendfile(this->file_descriptor, file_descriptor, &offset, size) == -1)
    {
        cerr << "Could not use sendfile system call!" << endl;
    }
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
