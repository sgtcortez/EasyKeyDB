#include "easykeyv1-clients.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

bool write_request(const EasyKeyV1Request* request, const Server* server)
{
    unsigned int size = 0;

    // The KnowNothing Protocol uses 1 byte plus 1 byte for the number of messages
    size += 2;

    for (unsigned int index = 0; index < request->messages_number; index++)
    {

        // Must add the 4 byte size of the message
        size+= 4;

        // Adds to the total request size the size of this message
        size += request->messages[index]->size;
    }

    // Calloc is the same as malloc, but, it also "resets" the returned pointer(fill with zeroes before returning)
    unsigned char * data = (unsigned char*) calloc(size, sizeof(char));

    // First byte is the protocol version
    data[0] = request->version;

    // Second byte is the number of messages
    data[1] = request->messages_number;


    // We need this variable, because we will write things dinamically
    unsigned int data_index = 2;

    for (unsigned int index = 0; index < request->messages_number; index++)
    {
        // For every messsage, we must serialize its size, then, copy the contents of the array to the data array
        Array* array = request->messages[index];

        // Serialize the 4 byte message size
        data[data_index++] = array->size;
        data[data_index++] = array->size >> 8;
        data[data_index++] = array->size >> 16;
        data[data_index++] = array->size >> 24;                

        // Copy the content to our buffer
        memcpy(data + data_index, array->array, array->size);

        // Updates the index
        data_index += array->size;
    }

    if (write(server->file_descriptor, data, size) < 0)
    {
        perror("write: ");
        return false;
    }
    return true;
}

EasyKeyV1Response* read_response(const Server* server)
{

    EasyKeyV1Response* response = (EasyKeyV1Response*) calloc(1, sizeof(EasyKeyV1Response));

    // The first read from socket, we can retrieve all usefull data
    /**
     * 0x00      - The first byte is the Know Nothing version
     * 0x01      - The second byte is the number of messages
     * 0x02/0x05 - The 4 bytes to represent the size status code. But only the lsb is used. We can ignore it
     * 0x06      - The 1 byte is the first message(1 byte to represent the status code)
     * 0x07/0x0A - The 4 bytes are the second message size.  

     * So, to make something simpler, and to not need to have an internal buffer to store content read from the socket, we can just make two reads
     * And, the second read can be to retrive the exact size of the second message     
     * NOTE: This is not performant, but easy to implment. Ought not be used in production
    */

    char buffer[11] = {0};        

    int read_size = read(server->file_descriptor, buffer, 11);
    if (read_size < 0) 
    {
        perror("read:");
        return NULL;
    }
    
    // The first response byte is the protocol version
    response->version = (enum KnowNothingProtocol) buffer[0];

    if (buffer[1] != 2) {
        fprintf(stderr, "Server returned an invalid number of messages!\n");
        return NULL;
    }

    // Status code is the first message
    // Since, every message must be after its size, and, the size is a 4 byte number and we are sure
    // that the status code is only one byte. We can ignore the 4 bytes of the size(bytes 3-6), and then, get the
    // value at byte 7
    response->status_code = (enum ResponseStatus) buffer[6];

    // We must retrieve the size of the value
    response->value.size = 0;

    response->value.size |= buffer[7];
    response->value.size |= (buffer[8]) << 8;
    response->value.size |= (buffer[9]) << 16;
    response->value.size |= (buffer[10]) << 24;

    // Allocated the exactly required memory
    response->value.array = (unsigned char* ) calloc(response->value.size, sizeof(char));
    
    // Reads the exaclty size 
    if (read(server->file_descriptor, response->value.array, response->value.size) < 0)
    {
        perror("read:");
        return NULL;
    }
    return response;
}

int get_socket_fd(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    return fd;
}

struct sockaddr_in create_socket(const Server server) {
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(struct sockaddr_in));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server.port);
    server_address.sin_addr.s_addr = inet_addr(server.ip_address);
    return server_address;
}

void print_server_response(const EasyKeyV1Response* reply)
{
    puts("\t----- Server Response -----");
    printf("\tKnow Nothing: \"%d\"\n", reply->version);
    printf("\tStatus Code: \"%d\"\n", reply->status_code);

    if (reply->value.size > 0)
    {
        // has response body
        printf("\tBody: \n");
        for (int index = 0; index < reply->value.size; index++)
        {
            printf("%c", reply->value.array[index]);
        }
        printf("\n");
    }
}

char * duplicate_string(const char* string)
{
    if (string == NULL)
    {
        return NULL;
    }
    size_t size = strlen(string);
    char *output = (char*)calloc(size + 1, sizeof(char));
    memcpy(output, string, size);
    return output;
}