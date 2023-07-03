#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <stdlib.h>     
#include <arpa/inet.h>  
#include <string.h>     
#include <stdio.h>      
#include <errno.h>      
#include <unistd.h>     

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9000

int socket_fd(void);

struct sockaddr_in create_socket(in_addr_t, unsigned int);

int main(int argc, char *argv[]){

    int socket_filedescriptor = socket_fd();
    struct sockaddr_in server_address = create_socket(inet_addr(SERVER_IP), SERVER_PORT);

    char msg[1024];
    if (argc < 2) 
    {
        strncpy(msg, "Hello, my name is client 123", sizeof(msg) / sizeof(msg[0]) - 1);
    }
    else 
    {
        strncpy(msg, argv[1], sizeof(msg) / sizeof(msg[0]) - 1);
    }

    connect(socket_filedescriptor, (struct sockaddr *) &server_address, sizeof(server_address));
    write(socket_filedescriptor, msg, strlen(msg));    

    char read_buffer[8 * 1024];        
    read(socket_filedescriptor, read_buffer, sizeof(read_buffer) / sizeof(read_buffer[0]));
    printf("Server response: \"%s\"\n", read_buffer);    

    close(socket_filedescriptor);
    puts("Closed connection...");
    return 0;
}

int socket_fd(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    return fd;
}

struct sockaddr_in create_socket(in_addr_t server_address, unsigned int server_port) {
    struct sockaddr_in server;
    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);
    server.sin_addr.s_addr = server_address;
    return server;
}