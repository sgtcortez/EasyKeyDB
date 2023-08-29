#ifndef _EASYKEY_V1_CLIENT__
#define _EASYKEY_V1_CLIENT__

#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <stdlib.h>     
#include <arpa/inet.h>  
#include <stdbool.h>

enum KnowNothingProtocol 
{
  V1 = 0x01
};

enum ResponseStatus
{
    OK = 0x01,
    CLIENT_ERROR = 0x02,
    SERVER_ERROR = 0x03,
};

typedef struct 
{
    const unsigned short port;
    const int file_descriptor;
    const char * ip_address;
} Server;

typedef struct 
{
    unsigned int size;
    unsigned char* array;
} Array;

typedef struct 
{
    enum KnowNothingProtocol version;
    enum ResponseStatus status_code;
    Array value;
} EasyKeyV1Response;

typedef struct
{
    enum KnowNothingProtocol version;
    unsigned char messages_number;
    Array ** messages; 
} EasyKeyV1Request;

/**
 * Writes a request to a EasyKey server
*/
bool write_request(const EasyKeyV1Request* request, const Server* server);


/**
 * Reads from the socket read buffer ...
 * Since the server sends the message size, we can read partially from the buffer 
*/
EasyKeyV1Response* read_response(const Server*);


/**
 * Creates the socket file descriptor
 * Which will act as our client
*/
int get_socket_fd(void);


/**
 * Creates the server socket variable which will be used to 
 * interact with the server
*/
struct sockaddr_in create_socket(const Server);


/**
 * Prints the response returned by the server
*/
void print_server_response(const EasyKeyV1Response*);

char * duplicate_string(const char *);

#endif /* _EASYKEY_V1_CLIENT__ */