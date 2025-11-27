#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "cmd.h"

#define SAFETY_PORT 10000
#define MAX_MSGS 100
#define MAX_LEN 1024

void clear_screen() {
    printf("\033[2J\033[H"); // Clear screen + move cursor to top-left
}

//idk if this works for generic cases
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

void send_signal(client_t * client, char * client_request){
    int rc = udp_socket_write(client->sd, &client->server_addr, client_request, BUFFER_SIZE);
    char * server_response;
    if (rc > 0)
    {
        int rc = udp_socket_read(client->sd, &client->responder_addr, server_response, BUFFER_SIZE);
        printf("server_response: %s", server_response);
    } 
    else{
        printf("Error! Message not sent!\n");
    }
}

void global_say(char message[], char client_messages[MAX_MSGS][MAX_LEN], int * client_messages_count, client_t *client){
    /*if (!client->connected){
        message_flash("Client Not Connected!");
        return;
    }*/
   
    // snprintf(client_messages[*client_messages_count], MAX_LEN + 6, "You: %s", message); // This is how we print to client messages.
    (*client_messages_count)++;
}

/*

void sayto(char who[], char message[], char client_messages[MAX_MSGS][MAX_LEN], int * client_messages_count, client_t *client){
    strncpy(client_messages[*client_messages_count], who, MAX_LEN - 1); // copy who
    strncpy(client_messages[*client_messages_count+1], message, sizeof(client_messages) - 1); // copy message
    printf("You: (private to %s) %s\n", client_messages[*client_messages_count], client_messages[*client_messages_count+1]); // should I add a "private" label in the chat?
    (*client_messages_count) += 2; // added "who", "msg"
}

void mute(char who[], char client_messages[MAX_MSGS][MAX_LEN], int * client_messages_count, client_t *client){
    strncpy(client_messages[*client_messages_count], who, MAX_LEN - 1); 
    print("You have muted %s\n", client_messages[*client_messages_count]); 
    *client_messages_count ++; 
}

void unmute(char who[], char client_messages[MAX_MSGS][MAX_LEN], int * client_messages_count, client_t *client){
    strncpy(client_messages[*client_messages_count], who, MAX_LEN - 1); 
    print("You have muted %s\n", client_messages[*client_messages_count]); 
    *client_messages_count ++; 
}

void rename(char name[], char client_messages[MAX_MSGS][MAX_LEN], int * client_messages_count, client_t *client){
    strncpy(client_messages[*client_messages_count], name, MAX_LEN - 1); 
    print("You have requested to rename to: %s\n", client_messages[*client_messages_count]); 
    *client_messages_count ++; 

}
*/ 

bool disconnect(){
    printf("You have disconnected\n"); 
    return true; 
}



void execute_command(command_t *command, client_t *client, char client_messages[MAX_MSGS][MAX_LEN], int * client_messages_count){
    int argsc = 1; 
    char display_to_client[MAX_LEN]; 
    switch(command->kind){
        case CONN:
            connect_to_server(command->args[0], client);
            break;
        case SAY:
            global_say(command->args[0], client_messages, client_messages_count, client);
            break; 
        case SAYTO:
            argsc = 2; 
            // if we want to store the activity, use snprintf instead of printf, 
            // and load it into the activity log (in client_types.h)
            printf("You: (private to %s) %s\n", command->args[0], command->args[1]); 
            // strncat(display_to_client, "You: (private to %s) %s\n", sizeof(display_to_client)-strlen(display_to_client)); 
            break; 
        case MUTE:
            printf("You have muted %s\n", command->args[0]);  
            break; 
        case UNMUTE:
            printf("You have unmuted %s\n", command->args[0]);  
            break; 
        case RENAME:
            // could fail (if there is already a same name)
            // come back later and fix the unique name problem
            printf("You have requested to rename to %s\n", command->args[0]);  
            break; 
        case DISCONN:
            disconnect(); 

        default:
            printf("Error, command type is undefined\n"); 
        
    }
    if((command->kind != CONN) && (command->kind != DISCONN)){
        char send_message[MAX_LEN];
        strncat(send_message, (char*) command->kind, sizeof(send_message)-strlen(send_message)-1); 
        strncat(send_message, " ", sizeof(send_message)-strlen(send_message)-1); 
        for(int i=0; i<argsc; i++){
            /*
            Error: The write_socket can only send a string, not an array of string
            strncpy(client_messages[*client_messages_count], command->args[i], MAX_LEN-1); 
            *client_messages_count ++; 
            */ 
           strncat(send_message, command->args[i],  sizeof(send_message)-strlen(send_message)-1); 
           strncat(send_message, " ", sizeof(send_message)-strlen(send_message)-1); 


        }
        strncat(send_message, "\0", sizeof(send_message)-strlen(send_message)-1); 
        // printf("%s\n", send_message); 
        send_signal(client, send_message); 
    }
    
}


// consider the client mode required thread
// listener thread and send thread

void *listener(void* args){
    while(1){

    }
}

void *sender(void* args){
    while(1){
        
    }

}

// sender does not need a queue, (in-built buffer)
// receiver strictly does not need a queue too. There is no synchronisation problem


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
            tokenise_input(input, args);//error check inpuct later

            command_t command;
            command_handler(&command, args);
            execute_command(&command, &client, messages, &message_count);
        }


        //listner thread stuff
        // create thread
        pthread_t listener_thread; 
        pthread_create(&listener_thread, NULL, listener, NULL); 
    }
    return 0;
}