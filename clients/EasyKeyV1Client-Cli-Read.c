/*
 This is just a command line client for Easy Key.   
 It only supports the read of a key.

 To run, you must execute ./program_name <name_of_key>
*/

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <stdlib.h>     
#include <arpa/inet.h>  
#include <string.h>     
#include <stdio.h>      
#include <errno.h>      
#include <unistd.h>     
#include <stdlib.h>

#include "easykeyv1-clients.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9000

int main(int argc, char** argv)
{
    const char* binary_name = argv[0];
    if (argc == 1)
    {
        fprintf(stderr, "Usage: %s key\n", binary_name);
        return 1;
    }

    const char* key_name = argv[1];

    int socket_fd = get_socket_fd();

    const Server server = {
        .port = SERVER_PORT, 
        .file_descriptor = socket_fd, 
        .ip_address = SERVER_IP
    };

    struct sockaddr_in server_address = create_socket(server);

    if (connect(socket_fd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
    {
        perror("connect:");
        exit(errno);
    }
    
    EasyKeyV1Request* request = (EasyKeyV1Request*) calloc(1, sizeof(EasyKeyV1Request));
    request->version = V1;
    request->messages_number = 1;
    request->messages = (Array**) calloc(1, sizeof(Array*));
    request->messages[0] = (Array*) calloc(1, sizeof(Array));
    request->messages[0]->size = strlen(key_name);
    request->messages[0]->array = (unsigned char*) duplicate_string(key_name);

    if (!(write_request(request, &server))) 
    {
        fprintf(stderr, "Could not send the request to the server ...\n");
        return 1;
    }

    EasyKeyV1Response* response = read_response(&server);
    print_server_response(response);
    
    bool success = response->status_code == OK; 
    free_easykey_request(request);
    free_easykey_response(response);

    if (!success)
    {
        return 1;
    }
    return 0;

}
