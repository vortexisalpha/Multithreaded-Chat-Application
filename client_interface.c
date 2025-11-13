#include <stdio.h>
#include <time.h>
#include "udp.h"

#define SAFETY_PORT 10000
#define MAX_MSGS 100
#define MAX_LEN 1024

void clear_screen() {
    printf("\033[2J\033[H"); // Clear screen + move cursor to top-left
}


void tokenise_input(char input[] ,char *args[]){
    char *token = strtok(input, "$");
    args[0] = token;
    printf("%s\n", token);
    token = strtok(NULL, "$");
    printf("%s\n", token);

    if (token != NULL){
        memmove(token, token + 1, strlen(token));
        args[1] = token; // removes the space
    }
}


void connect_to_server(char message[], client_t * client){
    // os has built in system to assign an unused port -> set udp_socket_open param to 0

    int sd = udp_socket_open(0);
    struct sockaddr_in server_addr, responder_addr;

    int rc = set_socket_addr(&server_addr, "127.0.0.1", SERVER_PORT);
    char client_request[BUFFER_SIZE], server_response[BUFFER_SIZE];
    strcpy(client_request, "TESTCON");
    rc = udp_socket_write(sd, &server_addr, client_request, BUFFER_SIZE);
    if (rc > 0)
    {
        int rc = udp_socket_read(sd, &responder_addr, server_response, BUFFER_SIZE);
        printf("server_response: %s", server_response);
    }

    strcpy(client->name, message);
    client->port = ntohs(responder_addr.sin_port);
    client->server_addr = server_addr;
    client->responder_addr = responder_addr;
    client->sd = sd;
    printf("%i\n",client->port);
}

void message_flash(char * message){
    clear_screen();
    for (int i = 0; i < 4; i++){
        printf("Message: %s\n", message);
        usleep(1000000); // sleep 1 sec
        clear_screen();
        fflush(stdout);
        usleep(1000000);
    }
}



void global_say(char message[], char client_messages[MAX_MSGS][MAX_LEN], int * client_messages_count, client_t *client){
    if (!client->connected){
        message_flash("Client Not Connected!");
        return;
    }
    snprintf(client_messages[*client_messages_count], MAX_LEN + 6, "You: %s", message); // This is how we print to client messages.
    (*client_messages_count)++;
}


void execute_command(command_t *command, client_t *client, char client_messages[MAX_MSGS][MAX_LEN], int * client_messages_count){
    switch(command->kind){
        case CONN:
            connect_to_server(command->message, client);
            break;
        case SAY:
            global_say(command->message, client_messages, client_messages_count, client);
    }
}

void command_handler(command_t *command, char * args[]){

    if(strcmp(args[0], "conn") == 0){
        command->kind = CONN;
    } else if(strcmp(args[0], "say") == 0){
        command->kind = SAY;
    } else if(strcmp(args[0], "sayto") == 0){
        command->kind = SAYTO;
    } else if(strcmp(args[0], "mute") == 0){
        command->kind = MUTE;
    } else if(strcmp(args[0], "unmute") == 0){
        command->kind = UNMUTE;
    } else if(strcmp(args[0], "rename") == 0){
        command->kind = RENAME;
    } else if(strcmp(args[0], "disconn") == 0){
        command->kind = DISCONN;
    } else if(strcmp(args[0], "kick") == 0){
        command->kind = KICK;
    }
    strcpy(command->message, args[1]);
}   
// client code
int main(int argc, char *argv[])
{   
    client_t client;
    setup_client(&client); // only sets client connected to false at the moment

    char messages[MAX_MSGS][MAX_LEN];
    int message_count = 0;

    char input[MAX_LEN];

    while(1){
        clear_screen(); // comment this out for testing prints to console
        printf("Chat Messages\n\n");

        for (int i = 0; i < message_count; i++){
            printf("%s\n", messages[i]);
        }

        printf("-----------------------------------\n");


        //sender thread stuff
        printf("> ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0; // remove \n when enter is pressed
        char *args[2];
    

        if (strcmp(input, ":q") == 0) break;

        if (message_count < MAX_MSGS){
            tokenise_input(input, args);//error check input later

            command_t command;
            command_handler(&command, args);
            execute_command(&command, &client, messages, &message_count);


          
        }


        //listner thread stuff
    }
    return 0;
}