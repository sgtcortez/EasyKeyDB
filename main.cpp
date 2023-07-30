#include "byte_buffer.hpp"
#include "easykey.hpp"
#include "server.hpp"
#include "database.hpp"

#include <sys/socket.h>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <csignal>

using namespace easykey;
using namespace std;
using namespace knownothing;

/**
 * The signal handler
 * man 7 signal for standard signals
 */
void signal_handler(int32_t signal);

void gracefully_termination();

Server* server_ptr = nullptr;
Database database;

void on_connection(const ClientSocket& client)
{
    cout << "A new client has just joined us!" << endl;

    struct linger linger_option;
    // enable
    linger_option.l_onoff = 0;
    // ten seconds
    linger_option.l_linger = 10;

    client.set_option(Socket::OptionValue<std::int32_t>(
        3, Socket::Option<int32_t>::TCP_NO_DELAY));
    client.set_option(Socket::OptionValue<struct linger>(
        linger_option, Socket::Option<struct linger>::LINGER));

    // The kernel doubles this size, to keep some caching & config things
    client.set_option(Socket::OptionValue<int32_t>(
        1024 * 8, Socket::Option<int32_t>::READ_BUFFER_SIZE_TYPE));

    const auto linger_opt_return =
        client.get_option(Socket::Option<struct linger>::LINGER);
    cout << "Is linger enabled? "
         << (linger_opt_return.value_type.l_onoff ? "true" : "false") << endl;
    cout << "Linger timeout:"
         << to_string(linger_opt_return.value_type.l_linger) << endl;
    cout << "Is no delay enabled? "
         << (client.get_option(Socket::Option<int32_t>::TCP_NO_DELAY).value_type
                 ? "true"
                 : "false")
         << endl;
    cout << "Is corking enabled? "
         << (client.get_option(Socket::Option<int32_t>::TCP_CORKING).value_type
                 ? "true"
                 : "false")
         << endl;
    cout << "Read buffer size: "
         << client.get_option(Socket::Option<int32_t>::READ_BUFFER_SIZE_TYPE)
                .value_type
         << endl;
}

void on_disconnected(const ClientSocket& client)
{
    cout << "The client: " << client.host_ip << ":" << client.port
         << " has just disconnected!" << endl;
}

vector<uint8_t> callback(const ClientSocket& client, ByteBuffer& buffer)
{
    cout << "Received data from client: " << client.host_ip << ":"
         << to_string(client.port) << endl;

    auto request = RequestMessage::read(buffer);
    if (request == nullptr)
    {
        string output_message =
            "The message does not follow the KnowNothing Specification!";
        vector<uint8_t> output(output_message.begin(), output_message.end());
        return ResponseMessage{Protocol::V1,
                               ResponseMessage::SERVER_ERROR,
                               output}
            .write();
    }
    // If its a read operation, the value will be empty
    if (request->value.size() == 0)
    {
        // Its a read operationn
        const auto result = database.read(request->key);
        if (result.empty())
        {
            string output_message =
                "Key \"" + request->key + "\" was not found!";
            vector<uint8_t> output(output_message.begin(),
                                   output_message.end());
            return ResponseMessage{Protocol::V1,
                                   ResponseMessage::CLIENT_ERROR,
                                   output}
                .write();
        }
        return ResponseMessage{Protocol::V1, ResponseMessage::OK, result}
            .write();
    }
    else
    {
        // its a write operation
        database.write(request->key, request->value);
        return ResponseMessage{Protocol::V1,
                               ResponseMessage::OK,
                               {'O', 'K', '!'}}
            .write();
    }
}

int main()
{
    auto server = Server(9000, 10, callback, on_connection, on_disconnected);

    server_ptr = &server;

    // https://en.cppreference.com/w/cpp/utility/program/signal
    /**
     * Register the signals to be handled!
     */
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    server.start();

    cout << "Finished!" << endl;

    return 0;
}

void signal_handler(int32_t signal)
{
    switch (signal)
    {
        case SIGINT:
        case SIGTERM:
            gracefully_termination();
            break;
        default:
            throw "Signal: " + to_string(signal) +
                " catched is not being handled!";
    }
}

void gracefully_termination()
{
    server_ptr->stop();
}