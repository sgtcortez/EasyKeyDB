#include "byte_buffer.hpp"
#include "easykey.hpp"
#include "server.hpp"
#include "socket.hpp"

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
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
Handler handler;

void on_connection(const ClientSocket& client)
{
    cout << "The client: " << client.host_ip << ":" << client.port
         << " has just connected!" << endl;
}

void on_disconnected(const ClientSocket& client)
{
    cout << "The client: " << client.host_ip << ":" << client.port
         << " has just disconnected!" << endl;
}

void on_message(ClientSocket& client)
{
    cout << "The client: " << client.host_ip << ":" << client.port
         << " sent a message!" << endl;
    handler.parse_request(client);
}

int main()
{
    auto server = Server(9000, 10, on_message, on_connection, on_disconnected);

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