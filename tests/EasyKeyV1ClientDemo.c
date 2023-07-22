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

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9000
#define PROTOCOL_VERSION 0x01

void menu(void);
void interactive_write(int fd);
void interactive_read(int fd);


typedef struct 
{
    unsigned int size;
    unsigned char* array;
} Array;

typedef struct 
{
    unsigned char know_nothing;
    unsigned char status_code;
    Array value;
} EasyKeyv1Reply;

Array * read_buffer = NULL;
unsigned int current_index = 0;

void read_filedescriptor(int fd);
EasyKeyv1Reply * get_reply(int fd);
void delete_reply(EasyKeyv1Reply* reply);
void print_server_response(EasyKeyv1Reply* reply);

int socket_fd(void);
struct sockaddr_in create_socket(in_addr_t, unsigned int);

int main(void)
{
    puts("EasyKeyV1 Demo!");
    puts("For this demo, just strings are supported!");
    menu();
    free(read_buffer);
    return 0;

}

void menu(void)
{

    int socket_filedescriptor = socket_fd();
    struct sockaddr_in server_address = create_socket(inet_addr(SERVER_IP), SERVER_PORT);

    if (connect(socket_filedescriptor, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
    {
        perror("Connect");
        exit(errno);
    }
    

    struct timeval timeout;
    timeout.tv_sec = 0; // 1 seconds
    timeout.tv_usec = 1000; // 1 milliseconds

    // Set socket receive timeout. 
    // Since the server will write reply right after, we do not want to wait for a timeout 
    if (setsockopt(socket_filedescriptor, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
        perror("setsockopt");
        close(socket_filedescriptor);
        exit(EXIT_FAILURE);
    }

    int option = 3;

    do 
    {
        puts("--------------------MENU--------------------");
        char *menu = "\n\n1. Write\n2. Read\n3. Exit\nYour option: ";
        printf("%s", menu);
        char number[4];
        fgets(number, 4, stdin);

        option = atoi(number);

        switch (option) {
            case 1:
                interactive_write(socket_filedescriptor);
                break;
            case 2:
                interactive_read(socket_filedescriptor);
                break;
            default:
                break;
        }
    } while(option != 3);

    close(socket_filedescriptor);
    puts("Disconneted!!!");
}

void interactive_write(int fd)
{
    puts("--------------------WRITE--------------------");

    unsigned int messsages_to_write = 0;
    printf("\tNumber of Messages to write: ");
    char message_buffer[4];
    fgets(message_buffer, 4, stdin);
    messsages_to_write = atoi(message_buffer);

    for (unsigned int message = 0; message < messsages_to_write; message++)
    {
        printf("\tKey %d: ", message);
        char key_buffer[1024];
        fgets(key_buffer, sizeof(key_buffer) / sizeof(key_buffer[0]), stdin);

        printf("\tValue %d: ", message);
        char value_buffer[1024 * 4];
        fgets(value_buffer, sizeof(value_buffer) / sizeof(value_buffer[0]), stdin);

        // remove the \n of the final index
        int key_size = strlen(key_buffer) - 1;
        int value_size = strlen(value_buffer) - 1;

        // protocol + 1 for number of messages + 1º message size + 1º message + 2º message size + 2º message
        const int message_size = 1 + 1 + sizeof(key_size) + key_size + sizeof(value_size) + value_size;

        // allocate the desired memory
        unsigned char* know_nothing = (unsigned char*) malloc(message_size);

        int index = 0;

        // protocol version
        know_nothing[index++] = PROTOCOL_VERSION;

        // There will be two messages. 
        // So we need to append them 
        know_nothing[index++] = 0x02; 

        // Serialize the 4 byte key size message in individual bytes
        know_nothing[index++] = key_size;
        know_nothing[index++] = key_size >> 8;
        know_nothing[index++] = key_size >> 16;
        know_nothing[index++] = key_size >> 24;

        // Copy the key content into the know nothing serialized data
        memcpy(know_nothing+index, key_buffer, key_size);

        // updates the index with the length of the key message size
        index += key_size;

        // Serialize the 4 byte value size message in individual bytes
        know_nothing[index++] = value_size;
        know_nothing[index++] = value_size >> 8;
        know_nothing[index++] = value_size >> 16;
        know_nothing[index++] = value_size >> 24;        

        // Copy the value content into the know nothing serialized data
        memcpy(know_nothing+index, value_buffer, value_size);

        if (write(fd, know_nothing, message_size) < 0)
        {
            perror("Write");
            exit(errno);
        }

        free(know_nothing);
    }

    for (unsigned int message = 0; message < messsages_to_write; message++)
    {
        printf("Response for message number: %d\n", message);
        EasyKeyv1Reply* server_response = get_reply(fd);
        print_server_response(server_response);
        delete_reply(server_response);
    }

    puts("--------------------WRITE--------------------");

}

void interactive_read(int fd)
{
    puts("--------------------READ--------------------");
    printf("\tKey: ");
    char key_buffer[1024];
    fgets(key_buffer, sizeof(key_buffer) / sizeof(key_buffer[0]), stdin);
    
    // remove the \n of the final index
    int key_size = strlen(key_buffer) - 1;

    // protocol + 1 for number of messages + 1º message size + 1º message
    const int message_size = 1 + 1 + sizeof(key_size) + key_size;

    // allocate the desired memory
    unsigned char* know_nothing = (unsigned char*) malloc(message_size);

    int index = 0;

    // protocol version
    know_nothing[index++] = PROTOCOL_VERSION;

    // There will be one message. 
    // So we need to append them 
    know_nothing[index++] = 0x01; 

    // Serialize the 4 byte key size message in individual bytes
    know_nothing[index++] = key_size;
    know_nothing[index++] = key_size >> 8;
    know_nothing[index++] = key_size >> 16;
    know_nothing[index++] = key_size >> 24;

    // Copy the key content into the know nothing serialized data
    memcpy(know_nothing+index, key_buffer, key_size);

    // updates the index with the length of the key message size
    index += key_size;

    if (write(fd, know_nothing, message_size) < 0)
    {
        perror("Write");
        exit(errno);
    }

    free(know_nothing);

    EasyKeyv1Reply* server_response = get_reply(fd);
    print_server_response(server_response);
    delete_reply(server_response);
    puts("--------------------READ--------------------");
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

void read_filedescriptor(int fd)
{

    // First read
    if (read_buffer == NULL) 
    {
        read_buffer = (Array *) malloc(sizeof(Array));
        // The first iteration 0 + bytes read = bytes read
        read_buffer->size = 0;

        // its safe to be null, because realloc deals with nulls pointers
        read_buffer->array = NULL; 
    }

    // the buffer to make successive calls to read
    char buffer[1024];

    // the buffer size
    const int buffer_size = sizeof(buffer) / sizeof(buffer[0]);
 
    do 
    {
        // read n bytes from file descriptor buffer
        int number_bytes_read = read(fd, buffer, buffer_size);

        // an error occurred
        if (number_bytes_read < 0)
        {
            // If no messages are sent or timeout is thrown
            return;
        }

        // the current size of the array
        int current = read_buffer->size;

        // the new total size of the array
        read_buffer->size = current + number_bytes_read;

        // realloc the pointer with more memory(also copies the current content)
        read_buffer->array = (unsigned char*) realloc(read_buffer->array, read_buffer->size);

        // copies the content of the new buffer into our array 
        memcpy(read_buffer->array + current, buffer, number_bytes_read);

        if (number_bytes_read < buffer_size)
        {
            // reached the end of the stream!
            break;
        }

    } while(1);
}


EasyKeyv1Reply * get_reply(int fd)
{

    /**
     * Read everything that is available in the socket at this time
     and store it at some local buffer
    */
    read_filedescriptor(fd);

    EasyKeyv1Reply* reply = (EasyKeyv1Reply *) malloc(sizeof(EasyKeyv1Reply));
    
    // get the protocol version
    reply->know_nothing = read_buffer->array[current_index++];
    
    // get the number of messages returned
    // must be 2
    int number_messages = read_buffer->array[current_index++];

    if (number_messages != 2)
    {
        fprintf(stderr, "Server returned an invalid number of messages!\n");
        exit(0);
    }

    // status code needs only 1 byte, but occupy 4 bytes to store the number size(4 bytes)
    // so we can advance
    current_index+=4;

    // get the request status code
    reply->status_code = read_buffer->array[current_index++];

    // Deserializes the second message size
    reply->value.size = 0;
    reply->value.size |= read_buffer->array[current_index++];
    reply->value.size |= (read_buffer->array[current_index++]) << 8;
    reply->value.size |= (read_buffer->array[current_index++]) << 16;
    reply->value.size |= (read_buffer->array[current_index++]) << 24;

    // Create the array to copy the content
    reply->value.array = (unsigned char *) malloc(sizeof(char) * reply->value.size);

    // copy the content into the array
    memcpy(reply->value.array, read_buffer->array+current_index, reply->value.size);
    
    // updates the current index to match the message value
    current_index+=reply->value.size;

    return reply;     

}

void delete_reply(EasyKeyv1Reply* reply)
{
    free(reply->value.array);
    free(reply);
}

void print_server_response(EasyKeyv1Reply* reply)
{
    puts("\t----- Server Response -----");
    printf("\tKnow Nothing: \"%d\"\n", reply->know_nothing);
    printf("\tStatus Code: \"%d\"\n", reply->status_code);

    if (reply->value.size > 0)
    {
        // has response body
        printf("\tBody: \"");
        for (int index = 0; index < reply->value.size; index++)
        {
            printf("%c", reply->value.array[index]);
        }
        printf("\"\n");
    }
}