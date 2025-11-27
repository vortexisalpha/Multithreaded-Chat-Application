#include <stdio.h>
#include <time.h>
#include "udp.h"
#include "client_types.h"
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
}

void global_say(char message[], char client_messages[MAX_MSGS][MAX_LEN], int * client_messages_count, client_t *client){
    /*if (!client->connected){
        message_flash("Client Not Connected!");
        return;
    }*/
    snprintf(client_messages[*client_messages_count], MAX_LEN + 6, "You: %s", message); // This is how we print to client messages.
    (*client_messages_count)++;
}


void execute_command(command_t *command, client_t *client, char client_messages[MAX_MSGS][MAX_LEN], int * client_messages_count){
    switch(command->kind){
        case CONN:
            connect_to_server(command->args[0], client);
            break;
        case SAY:
            global_say(command->args[0], client_messages, client_messages_count, client);
        default:
        
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

void join_with_spaces(char *tokenised_command[MAX_COMMAND_LEN], char *joint_msg) {
    joint_msg[0] = '\0';

    for (int i = 0; tokenised_command[i] != NULL; i++) {
        strcat(joint_msg, tokenised_command[i]);

        //add space between words except last
        if (tokenised_command[i+1] != NULL)
            strcat(joint_msg, " ");
    }
}
void *queue_manager(void* arg){
    cli_queue_manager_args_t* qm_args = (cli_queue_manager_args_t *)arg;
    
    client_t * client = qm_args->client;
    while(1){
        char * tokenised_command[MAX_COMMAND_LEN];

        q_pop(qm_args->task_queue, tokenised_command, NULL); // pop command from front of command queue (includes sleep wait for queue nonempty)

        char * server_request;
        join_with_spaces(tokenised_command, server_request)
        spawn_execute_command_threads(client->sd, &cur_command);  
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
    }
    return 0;
}