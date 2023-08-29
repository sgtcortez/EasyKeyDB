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
#include <sys/wait.h>

#include "easykeyv1-clients.h"

#define CLI_WRITE_BIN "./cli-write.out"
#define CLI_READ_BIN "./cli-read.out"

void fork_p(bool read);

int main(void)
{
    puts("EasyKeyV1 Interactive Demo!");
    puts("For this demo, just strings are supported!");

    int option = 3;

    do 
    {
        puts("------------------ MENU ------------------");
        printf("%s", "\n\n1. Write\n2. Read\n3. Exit\nYour option: ");
        char number[4];
        fgets(number, 4, stdin);

        option = atoi(number);        

        switch (option)
        {
            case 1:
                fork_p(false);
                break;
            case 2:
                fork_p(true);
                break;
            default:
                break;
        }


    } while(option != 3);

    return 0;
}


void fork_p(bool read) 
{
    printf("\tKey: ");
    char key_buffer[1024];
    fgets(key_buffer, sizeof(key_buffer) / sizeof(key_buffer[0]), stdin);
    // Removes the \n at the end of the message
    key_buffer[strlen(key_buffer) - 1] = 0;

    pid_t process_id = fork();
    if (process_id == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (process_id > 0)
    {
        // in the parent process, we need to wait for the child
        waitpid(process_id, NULL, 0);    
    }
    else 
    {
        if (read)
        {
            char* args[3];

            args[0] = duplicate_string(CLI_READ_BIN);
            args[1] = duplicate_string(key_buffer);
            args[2] = NULL;
            execv(CLI_READ_BIN, args);

            // Execv only returns if an error occured
            exit(1);
        }   
        else 
        {
            printf("\tValue: ");
            char value_buffer[1024];
            fgets(value_buffer, sizeof(value_buffer) / sizeof(value_buffer[0]), stdin);
            // Removes the \n at the end of the message
            value_buffer[strlen(value_buffer) - 1] = 0;

            char* args[5];

            args[0] = duplicate_string(CLI_WRITE_BIN);
            args[1] = duplicate_string(key_buffer);
            args[2] = duplicate_string(value_buffer);
            args[3] = duplicate_string("--file=no");
            args[4] = NULL;
            execv(CLI_WRITE_BIN, args);

            // Execv only returns if an error occured
            exit(1);

        }


    }



}

