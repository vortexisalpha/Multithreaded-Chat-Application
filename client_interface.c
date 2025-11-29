#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "udp.h"
#include "client_types.h"
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


//reads socket and appends to queue
void *cli_listener(void *arg){
    cli_listener_args_t* listener_args = (cli_listener_args_t*)arg;
    //takes in the queue and adds the server response command to it,
    //queue is of form: [queue_node_t,...] each queue_node_t has msg and from addr
    printf("Client is listening on server port: %d\n", SERVER_PORT);
    while (1) {
        // Storage for response from server
        char server_response[BUFFER_SIZE];
        client_t * = lister_args->client;

        int rc = udp_socket_read(client_ptr->sd, &client_ptr->responder_addr, server_response, BUFFER_SIZE);

        if (rc > 0){
            q_append(listener_args->task_queue, server_response, NULL); // this includes queue full sleep for thread
        }
    }
}

//chat display args need message buffer, message_count pointer, client_t ptr
//sleeps on no input
void *chat_display_thread(void *arg){
    chat_display_args_t* chat_args = (chat_display_args_t*)arg;

    while(1){
        clear_screen(); // comment this out for testing prints to console
        printf("Chat Messages\n\n");

        for (int i = 0; i < *chat_args->message_count; i++){
            printf("%s\n", chat_args->messages[i]);
        }

        printf("-----------------------------------\n");

        printf("> ");
        //NEED TO IMPLEMENT COND SIGNAL WHEN MESSAGES IS UPDATED
        //if pthread_cond_signal does not say that we must update the message display{:
        // wait for input:
        fgets(input, sizeof(input), stdin);

        input[strcspn(input, "\n")] = 0; // remove \n when enter is pressed
        client_t * client = chat_args->client;
        int rc = udp_socket_write(client->sd, &client->server_addr, &input[0], BUFFER_SIZE);
    
        if (strcmp(input, ":q") == 0) break; // replace this with end_all_threads() function
    }
}

/*
void join_with_spaces(char *tokenised_command[MAX_COMMAND_LEN], char *joint_msg) {
    joint_msg[0] = '\0';

    for (int i = 0; tokenised_command[i] != NULL; i++) {
        strcat(joint_msg, tokenised_command[i]);

        //add space between words except last
        if (tokenised_command[i+1] != NULL)
            strcat(joint_msg, " ");
    }
}
*/
void execute_server_command(comand_t cmd){
}

void *cli_queue_manager(void* arg){
    cli_queue_manager_args_t* qm_args = (cli_queue_manager_args_t *)arg;
    
    client_t * client = qm_args->client;
    while(1){
        char * tokenised_command[MAX_COMMAND_LEN];

        q_pop(qm_args->task_queue, tokenised_command, NULL); // pop command from front of command queue (includes sleep wait for queue nonempty)

        char client_request[BUFFER_SIZE];
        command_t cmd;
        command_handler(&cmd, tokenised_command); // fill out cur_command
        execute_server_command(cmd);
    }
}



// client code
int main(int argc, char *argv[])
{      
    //setup code:
    client_t client;
    Queue task_queue;

    setup_client(&client); // only sets client connected to false at the moment
    queue_init(&task_queue);

    char messages[MAX_MSGS][MAX_LEN];
    int message_count = 0;

    char input[MAX_LEN];

    //spawn listner
    pthread_t listener_thread;
    listener_args_t *cli_listener_args = malloc(sizeof(listener_args_t));
    setup_listener_args(listener_args, sd, &task_queue);
    pthread_create(&listener_thread, NULL, listener, listener_args);
    pthread_detach(listener_thread); //?

    //spawn queue manager thread
    pthread_t queue_manager_thread;
    queue_manager_args_t *cli_queue_manager_args = malloc(sizeof(queue_manager_args_t));
    setup_queue_manager_args(queue_manager_args,&task_queue, sd, &head, &tail);
    pthread_create(&queue_manager_thread, NULL, queue_manager, queue_manager_args);
    pthread_detach(queue_manager_thread);

    return 0;
}