/*
 This is just a command line client for Easy Key.   
 It only supports the write of a key.

 To run, you must execute ./program_name <name_of_key> value --file=[yes|no]

 If --file=yes, then, we read the content pointed by value and sends it to the server
 Otherwise, we just send the value as the value to the server
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
#include <sys/stat.h>

#include "easykeyv1-clients.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9000

int main(int argc, char** argv)
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s keyname value --file=[yes|no]\n", argv[0]);
        return 1;
    }
    
    char *key_name = argv[1];
    char *value = argv[2];
    bool is_file = strncmp(argv[3], "--file=yes", 10) == 0;

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
    request->messages_number = 2;
    request->messages = (Array**) calloc(request->messages_number, sizeof(Array*));
    
    request->messages[0] = (Array*) calloc(1, sizeof(Array));
    request->messages[0]->size = strlen(key_name);
    request->messages[0]->array = (unsigned char*) duplicate_string(key_name);    

    if (is_file)
    {
        FILE *openfile = fopen(value, "rb");
        if (openfile == NULL)
        {
            perror("fopen:");
            return 1;
        }

        // Set the offset to the end of the file
        fseek(openfile, 0, SEEK_END);

        request->messages[1] = (Array*) calloc(1, sizeof(Array));
        request->messages[1]->size = ftell(openfile);
        request->messages[1]->array = (unsigned char*) calloc(1, request->messages[1]->size);

        // to read from the beginning, we must set the file offset to the beginning
        fseek(openfile, 0, SEEK_SET);

        // reads the content of the file to the buffer
        const size_t bytes_read = fread(request->messages[1]->array, sizeof(char), request->messages[1]->size, openfile);
        if (bytes_read < 0)
        {
            perror("fread: ");
            return 1;
        }
        if (bytes_read != request->messages[1]->size)
        {
            fprintf(stderr, "fread requested: %d but read only: %lu! This indicates an error!\n", request->messages[1]->size, bytes_read);
            return 1;
        }

        // Close the opened file
        fclose(openfile);
    }
    else 
    {
        // not a file, just copy what the client written in the command line argument
        request->messages[1] = (Array*) calloc(1, sizeof(Array));
        request->messages[1]->size = strlen(value);
        request->messages[1]->array = (unsigned char*)value;
    }


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