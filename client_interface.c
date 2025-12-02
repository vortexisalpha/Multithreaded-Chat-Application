#include <stdlib.h>
#include <sys/select.h>
#include "custom_hash_table.h"
#include "udp.h"
#include "cmd.h"
#include "client_types.h"
#include <pthread.h>

#define SAFETY_PORT 10000


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


// Initialize socket without authenticating
void init_client_socket(client_t * client){
    // Open socket - OS assigns unused port
    int sd = udp_socket_open(0);
    if (sd < 0) {
        printf("Error: Failed to open socket\n");
        return;
    }
    
    // Set up server address
    struct sockaddr_in server_addr;
    int rc = set_socket_addr(&server_addr, "127.0.0.1", SERVER_PORT);
    if (rc < 0) {
        printf("Error: Failed to set server address\n");
        close(sd);
        return;
    }
    
    // Save socket info (but don't set connected=true yet)
    client->sd = sd;
    client->server_addr = server_addr;
    printf("Socket initialized. Ready to connect.\n");
}

// Send connection request to server
void send_connect_request(client_t * client, char* username){
    char client_request[BUFFER_SIZE];
    snprintf(client_request, BUFFER_SIZE, "conn$ %s", username);
    
    int rc = udp_socket_write(client->sd, &client->server_addr, client_request, strlen(client_request) + 1);
    if (rc <= 0) {
        printf("Error: Failed to send connection request\n");
    } else {
        printf("Connection request sent for username: %s\n", username);
    }
}

// OLD function - kept for reference, will be removed later
void connect_to_server(char message[], client_t * client){
    // os has built in system to assign an unused port -> set udp_socket_open param to 0

    int sd = udp_socket_open(0);
    struct sockaddr_in server_addr, responder_addr;

    int rc = set_socket_addr(&server_addr, "127.0.0.1", SERVER_PORT);
    char client_request[BUFFER_SIZE], server_response[BUFFER_SIZE];
    strcpy(client_request, "conn$ alice");
    rc = udp_socket_write(sd, &server_addr, client_request, BUFFER_SIZE);
    if (rc > 0)
    {
        int rc = udp_socket_read(sd, &responder_addr, server_response, BUFFER_SIZE);
        printf("server_response: %s", server_response);
        strcpy(client->name, message);
        client->server_addr = server_addr;
        client->responder_addr = responder_addr;
        client->sd = sd;
    } 
    
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

//reads socket and appends to queue
void *cli_listener(void *arg){
    cli_listener_args_t* listener_args = (cli_listener_args_t*)arg;
    //takes in the queue and adds the server response command to it,
    //queue is of form: [queue_node_t,...] each queue_node_t has msg and from addr
    
    while (1) {
        // Storage for response from server
        char server_response[BUFFER_SIZE];
        client_t * client_ptr = listener_args->client;

        int rc = udp_socket_read(client_ptr->sd, &client_ptr->responder_addr, server_response, BUFFER_SIZE);

        if (rc > 0){
            q_append(listener_args->task_queue, server_response, NULL); // this includes queue full sleep for thread
        }
    }
    return NULL;
}

// chat display thread with adaptive growing display
void *chat_display(void *arg){
    chat_display_args_t* chat_args = (chat_display_args_t*)arg;
    
    //enter alternate screen buffer
    printf("\033[?1049h");
    
    //initial screen setup
    printf("\033[2J\033[H"); // clear screen and move to home
    printf("Chat Messages:\n\n");
    printf("---------------------\n");
    printf("> ");
    fflush(stdout);

    while(1){
        //wait for new messages
        pthread_mutex_lock(chat_args->messages_mutex);
        pthread_cond_wait(chat_args->messages_cond, chat_args->messages_mutex);
        
        //redraw entire screen from top
        printf("\033[2J\033[H"); //clear and go to top
        
        //draw header
        printf("Chat Messages:\n\n");
        
        //print all messages
        int total_messages = *chat_args->message_count;
        hash_node_t **mute_table = chat_args->client->mute_table;
        for (int i = 0; i < total_messages; i++){
            char username[NAME_SIZE];
            extract_username(chat_args->messages[i], username, NAME_SIZE);
            if(contains(mute_table, username)) continue;
            printf("%s\n", chat_args->messages[i]);
        }
        
        printf("\n---------------------\n");
        printf("> ");
        fflush(stdout);
        
        pthread_mutex_unlock(chat_args->messages_mutex);
    }
    
    return NULL;
}

void *user_input_pre_connection(void *arg){
    user_input_args_t* input_args = (user_input_args_t*)arg;
    char input[MAX_LEN];
    client_t* client = input_args->client;
    
    //give display thread time to set up screen
    sleep(1);

    while(1){
        //read input
        if (fgets(input, sizeof(input), stdin) != NULL) {
            input[strcspn(input, "\n")] = 0; // remove newline
            
            if (strcmp(input, ":q") == 0) {
                //exit alternate screen buffer and return to normal terminal
                printf("\033[?1049l");
                printf("Exiting...\n");
                exit(0);
            }
            
            //send to server
            if (strlen(input) > 0) {
                if_connect_command_connect(input);
            }
        }
    }
    
    return NULL;
}

// user input thread handles user typing and sending
void *user_input(void *arg){
    user_input_args_t* input_args = (user_input_args_t*)arg;
    char input[MAX_LEN];
    client_t* client = input_args->client;
    
    //give display thread time to set up screen
    sleep(1);

    while(1){
        //read input
        if (fgets(input, sizeof(input), stdin) != NULL) {
            input[strcspn(input, "\n")] = 0; // remove newline
            
            if (strcmp(input, ":q") == 0) {
                //exit alternate screen buffer and return to normal terminal
                printf("\033[?1049l");
                printf("Exiting...\n");
                exit(0);
            }
            
            //send to server
            if (strlen(input) > 0 && client->connected) {
                udp_socket_write(client->sd, &client->server_addr, input, strlen(input) + 1);
            }
        }
    }
    
    return NULL;
}

// Handle connection success response from server
void execute_connect_response(command_t *cmd, client_t * client, pthread_mutex_t* message_mutex, pthread_cond_t* message_update_cond){
    char* username = cmd->args[0];
    
    pthread_mutex_lock(&client->connection_mutex);
    
    // Set connection state
    client->connected = true;
    strcpy(client->name, username);
    
    // Wake up the post-connection input thread
    pthread_cond_signal(&client->connection_cond);
    
    pthread_mutex_unlock(&client->connection_mutex);
    
    // Trigger chat display update to show new prompt
    pthread_mutex_lock(message_mutex);
    pthread_cond_signal(message_update_cond);
    pthread_mutex_unlock(message_mutex);
    
    printf("Successfully connected as: %s\n", username);
}

// Handle connection failure response from server
void execute_connect_failed(command_t *cmd, client_t * client){
    char* error_message = cmd->args[0];
    printf("Connection failed: %s\n", error_message);
    printf("Please try again with a different username.\n");
}

// Handle disconnect response from server
void execute_disconnect_response(client_t * client, int * message_count, pthread_mutex_t* message_mutex, pthread_cond_t* message_update_cond){
    pthread_mutex_lock(&client->connection_mutex);
    
    // Set connection state to false
    client->connected = false;
    
    // Close the socket
    if (client->sd > 0) {
        close(client->sd);
        client->sd = 0;
    }
    
    // Wake up the pre-connection input thread
    pthread_cond_signal(&client->connection_cond);
    
    pthread_mutex_unlock(&client->connection_mutex);
    
    // Clear chat history
    pthread_mutex_lock(message_mutex);
    *message_count = 0;
    pthread_cond_signal(message_update_cond);
    pthread_mutex_unlock(message_mutex);
    
    printf("Disconnected from server.\n");
}

//execute server response. e.g say command
void execute_server_command(command_t *cmd, client_t * client,  int * message_count, char (* messages)[MAX_LEN], pthread_mutex_t* message_mutex, pthread_cond_t* message_update_cond){
    switch(cmd->kind){
        case CONN_SUCCESS:
            execute_connect_response(cmd, client, message_mutex, message_update_cond);
            break;
        case CONN_FAILED:
            execute_connect_failed(cmd, client);
            break;
        case DISCONN_RESPONSE:
            execute_disconnect_response(client, message_count, message_mutex, message_update_cond);
            break;
        case SAY:
            say_exec(cmd, message_count, messages, message_mutex, message_update_cond);
            break;
        case MUTE:
            mute_exec(cmd, client, message_mutex, message_update_cond);
            break;
        case UNMUTE:
            unmute_exec(cmd, client, message_mutex, message_update_cond);
            break;
        default:
            break;
    }
}

//handles each command
void* handle_cli_side_cmd(void* args){
    handle_cli_side_cmd_args_t* cli_side_args = (handle_cli_side_cmd_args_t *)args;
    
    command_t cmd;
    command_handler(&cmd, cli_side_args->tokenised_command);
    execute_server_command(&cmd, cli_side_args->client, cli_side_args->message_count, cli_side_args->messages, cli_side_args->messages_mutex, cli_side_args->messages_cond);
    
    //clean up
    if (cli_side_args->tokenised_command != NULL) {
        for (int i = 0; i < MAX_COMMAND_LEN && cli_side_args->tokenised_command[i] != NULL; i++) {
            free(cli_side_args->tokenised_command[i]);
        }
        free(cli_side_args->tokenised_command);
    }
    free(cli_side_args);
    
    return NULL;
}

//queue manager thread
//decodes request coming in to server and calls execute_server_command to execute on client side
void *cli_queue_manager(void* arg){
    cli_queue_manager_args_t* qm_args = (cli_queue_manager_args_t *)arg;
    
    client_t * client = qm_args->client;
    while(1){
        char * tokenised_command_temp[MAX_COMMAND_LEN] = {NULL}; 

        q_pop(qm_args->task_queue, tokenised_command_temp, NULL); // pop command from front of command queue (includes sleep wait for queue nonempty)
        //POTENTIAL RACE CONDITION CONSERN: THREAD SWITCH HERE AND QUEUE[i] REPLACED REQUIRES FIXING

        //copy the tokenised_command to heap memory to avoid race condition
        char ** tokenised_command_copy = malloc(MAX_COMMAND_LEN * sizeof(char*));
        for (int i = 0; i < MAX_COMMAND_LEN; i++) {
            if (tokenised_command_temp[i] != NULL) {
                tokenised_command_copy[i] = malloc(strlen(tokenised_command_temp[i]) + 1);
                strcpy(tokenised_command_copy[i], tokenised_command_temp[i]);
            } else {
                tokenised_command_copy[i] = NULL;
            }
        }

        pthread_t t;
        handle_cli_side_cmd_args_t *handle_cli_side_cmd_args = malloc(sizeof(handle_cli_side_cmd_args_t));
        setup_handle_cli_side_cmd_args(handle_cli_side_cmd_args, client, qm_args->messages, qm_args->message_count, qm_args->messages_mutex, qm_args->messages_cond, tokenised_command_copy);
        pthread_create(&t, NULL, handle_cli_side_cmd, handle_cli_side_cmd_args);
        pthread_detach(t);
    }
    return NULL;
}



// client code
int main(int argc, char *argv[])
{      
    //setup code:
    client_t client;
    Queue task_queue;

    setup_client(&client); // only sets client connected to false at the moment
    queue_init(&task_queue);
    connect_to_server("John\0", &client);
    char messages[MAX_MSGS][MAX_LEN];
    int message_count = 0;

    //initialize mutex and condition variable for message synchronization
    pthread_mutex_t messages_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t messages_cond = PTHREAD_COND_INITIALIZER;

    //spawn chat display thread
    pthread_t chat_display_thread;
    chat_display_args_t *chat_display_args = malloc(sizeof(chat_display_args_t));
    setup_chat_display_args(chat_display_args, &client, messages, &message_count, &messages_mutex, &messages_cond);
    pthread_create(&chat_display_thread, NULL, chat_display, chat_display_args);
    pthread_detach(chat_display_thread);

    //spawn user input sender thread
    pthread_t user_input_thread;
    user_input_args_t *user_input_args = malloc(sizeof(user_input_args_t));
    setup_user_input_args(user_input_args, &client);
    pthread_create(&user_input_thread, NULL, user_input, user_input_args);
    pthread_detach(user_input_thread);

    //spawn listner
    pthread_t listener_thread;
    cli_listener_args_t *cli_listener_args = malloc(sizeof(cli_listener_args_t));
    setup_cli_listner_args(cli_listener_args, &client, &task_queue);
    pthread_create(&listener_thread, NULL, cli_listener, cli_listener_args);
    pthread_detach(listener_thread); //?

    //spawn queue manager thread
    pthread_t queue_manager_thread;
    cli_queue_manager_args_t *cli_queue_manager_args = malloc(sizeof(cli_queue_manager_args_t));
    setup_cli_queue_manager_args(cli_queue_manager_args, &client, &task_queue, messages, &message_count, &messages_mutex, &messages_cond);
    pthread_create(&queue_manager_thread, NULL, cli_queue_manager, cli_queue_manager_args);
    pthread_detach(queue_manager_thread);
    

    pthread_exit(NULL); // exit main thread but keep other bg threads

    return 0;
}