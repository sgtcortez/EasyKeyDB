#pragma once

#include <bits/stdint-uintn.h>
#include <cstdint>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <memory>
#include <vector>

namespace easykey 
{

    /**
     * Forward declarations
    */
    class ServerSocket;
    class ClientSocket;    

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
        public:
            Socket(const std::int32_t file_descriptor, const struct sockaddr_in address);
            ~Socket();

            // TOOD: THis must not be public!
            const std::int32_t file_descriptor;

        protected:
            const struct sockaddr_in address;
    };

    class ServerSocket : public Socket
    {
        public:
            ServerSocket(const std::int32_t file_descriptor, const struct sockaddr_in address);
            void assign_address() const;
            void set_available(std::uint16_t backlog_queue) const;
            const ClientSocket * accept_connection() const;

    };

    class ClientSocket : public Socket 
    {
        public:
            ClientSocket(const std::int32_t file_descriptor, const struct sockaddr_in address);
            void send(std::vector<uint8_t> message) const;
            std::vector<uint8_t> get() const;
    };

    
    /**
     * This class is used to exchange data between client & server
     * The client connects, sends a message, than the server replies.   
     * The main basic of client-server architeture.
    */
    struct Dispatcher 
    {
        public:
            virtual std::string name() const = 0;
            virtual std::vector<std::uint8_t> exchange(std::vector<std::uint8_t> client_request) = 0;
    };


    class Server
    {
        
        public:
            Server(const std::uint16_t port, const std::uint16_t pending_connections, Dispatcher& dispatcher);        
            void start();
            void stop();
        private:
            const std::uint16_t port;
            const std::uint16_t pending_connections;
            const ServerSocket socket;
            Dispatcher& dispatcher;

            // A SIGTERM/SIGINT signal set this to false
            bool running;

            /**
             A new connection arrived
            */
            void handle_new_connection();

            /**
             * Check for idle connections and then remove them for current_connectins map
            */
            void check_idle_connections();

            /**
             * Handle client disconnected 
            */
            void handle_client_disconnected(std::int32_t file_descriptor);

            /**
             * Handle the read request
            */
            void handle_request(const ClientSocket* client) const;
  
            /**
             Keep track of the current client connections 
            */
            std::unordered_map<std::int32_t, std::unique_ptr<const ClientSocket>> current_connections;

    };

};