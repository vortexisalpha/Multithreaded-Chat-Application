#include <stdlib.h>
#include <sys/select.h>
#include "custom_hash_table.h"
#include "udp.h"
#include "cmd.h"
#include "client_types.h"

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


//initialize socket without authenticating
void init_client_socket(client_t * client){
    int sd = udp_socket_open(0);
    if (sd < 0) {
        printf("Error: Failed to open socket\n");
        return;
    }
    
    struct sockaddr_in server_addr;
    int rc = set_socket_addr(&server_addr, "127.0.0.1", SERVER_PORT);
    if (rc < 0) {
        printf("Error: Failed to set server address\n");
        close(sd);
        return;
    }
    
    //save socket info but dont set connected = true yet. We have to wait for server confirmation
    client->sd = sd;
    client->server_addr = server_addr;
    printf("Socket initialized. Ready to connect.\n");
}

void send_connect_request(client_t * client, char* username){
    char client_request[BUFFER_SIZE];
    snprintf(client_request, BUFFER_SIZE, "conn$ %s", username);
    
    printf("[DEBUG] Sending '%s' to port %d (sd=%d)\n", client_request, SERVER_PORT, client->sd);
    int rc = udp_socket_write(client->sd, &client->server_addr, client_request, strlen(client_request) + 1);
    if (rc <= 0) {
        printf("Error: Failed to send connection request (rc=%d)\n", rc);
    } else {
        printf("Connection request sent for username: %s (%d bytes sent)\n", username, rc);
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

//chat display thread with adaptive growing display
void *chat_display(void *arg){
    chat_display_args_t* chat_args = (chat_display_args_t*)arg;
    
    //enter alternate screen buffer
    printf("\033[?1049h");
    
    //initial screen setup
    printf("\033[2J\033[H"); // clear screen and move to home
    printf("Chat Messages:\n\n");
    printf("---------------------\n");
    
    //display initial prompt based on connection state
    pthread_mutex_lock(&chat_args->client->connection_mutex);
    if (chat_args->client->connected) {
        printf("[%s] > ", chat_args->client->name);
    } else {
        printf("[Not connected] > ");
    }
    pthread_mutex_unlock(&chat_args->client->connection_mutex);
    fflush(stdout);

    while(1){
        //wait for new messages or connection state changes
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
        
        //display prompt based on connection state
        pthread_mutex_lock(&chat_args->client->connection_mutex);
        if (chat_args->client->connected) {
            printf("[%s] > ", chat_args->client->name);
        } else {
            printf("[Not connected] > ");
        }
        pthread_mutex_unlock(&chat_args->client->connection_mutex);
        fflush(stdout);
        
        pthread_mutex_unlock(chat_args->messages_mutex);
    }
    
    return NULL;
}

//pre-connection user input thread handles input before connected
void *user_input_pre_connection(void *arg){
    user_input_args_t* input_args = (user_input_args_t*)arg;
    char input[MAX_LEN];
    client_t* client = input_args->client;
    
    //give display thread time to set up screen
    sleep(1);

    while(1){
        //check if client connected
        pthread_mutex_lock(&client->connection_mutex);
        while(client->connected == true){
            //sleep until disconnected
            pthread_cond_wait(&client->connection_cond, &client->connection_mutex);
        }
        pthread_mutex_unlock(&client->connection_mutex);
        
        //double check before blocking on fgets in case connection happened during unlock
        pthread_mutex_lock(&client->connection_mutex);
        bool is_connected = client->connected;
        pthread_mutex_unlock(&client->connection_mutex);
        
        if (is_connected) {
            continue; //connection happene so loop back to wait
        }
        
        if (fgets(input, sizeof(input), stdin) != NULL) {
            input[strcspn(input, "\n")] = 0; // remove newline
            
            if (strcmp(input, ":q") == 0) {
                //exit alternate screen buffer and return to normal terminal
                printf("\033[?1049l");
                printf("Exiting...\n");
                exit(0);
            }
            
            //check if it's a connection command
            if (strncmp(input, "conn$ ", 6) == 0) {
                //extract username as everything after "conn$ "
                char* username = input + 6;
                if (strlen(username) > 0) {
                    //reinitialize socket if closed after disconnect
                    if (client->sd == 0) {
                        init_client_socket(client);
                    }
                    send_connect_request(client, username);
                    //give server time to respond before reading next input
                    usleep(100000); //100ms
                } else {
                    printf("Error: Username cannot be empty.\n");
                    printf("[Not connected] > ");
                    fflush(stdout);
                }
            } else if (strlen(input) > 0) {
                //not a valid pre connection command
                printf("Error: Not connected.\n");
                printf("[Not connected] > ");
                fflush(stdout);
            }
        }
    }
    
    return NULL;
}

//post connection user input thread handles input after connected
void *user_input(void *arg){
    user_input_args_t* input_args = (user_input_args_t*)arg;
    char input[MAX_LEN];
    client_t* client = input_args->client;
    
    //give display thread time to set up screen
    sleep(1);

    while(1){
        //check if client connected
        pthread_mutex_lock(&client->connection_mutex);
        while(client->connected == false){
            //sleep until connected
            pthread_cond_wait(&client->connection_cond, &client->connection_mutex);
        }
        pthread_mutex_unlock(&client->connection_mutex);
        
        //double-check before blocking on fgets in case disconnection happened during unlock
        pthread_mutex_lock(&client->connection_mutex);
        bool is_connected = client->connected;
        pthread_mutex_unlock(&client->connection_mutex);
        
        if (!is_connected) {
            continue; //disconnection happened, loop back to wait
        }
        
        //read input only when connected
        if (fgets(input, sizeof(input), stdin) != NULL) {
            input[strcspn(input, "\n")] = 0; // remove newline
            
            if (strcmp(input, ":q") == 0) {
                //send disconnect before quitting
                char disconn_msg[] = "disconn$";
                udp_socket_write(client->sd, &client->server_addr, disconn_msg, strlen(disconn_msg) + 1);
                sleep(1); // give server time to process
                
                //exit alternate screen buffer and return to normal terminal
                printf("\033[?1049l");
                printf("Exiting...\n");
                exit(0);
            }
            
            //check if disconnect command
            if (strcmp(input, "disconn$") == 0) {
                //send disconnect to server
                udp_socket_write(client->sd, &client->server_addr, input, strlen(input) + 1);
                printf("Disconnecting...\n");
                //server will send disconnresponse$ which will trigger execute_disconnect_response()
                //that function will set connected=false and signal condition variable
                //so this thread will loop back and go to sleep
            }
            //send other commands to server
            else if (strlen(input) > 0) {
                udp_socket_write(client->sd, &client->server_addr, input, strlen(input) + 1);
                //reprint prompt for next command
                //printf("[%s] > ", client->name);
                //fflush(stdout);
            }
        }
    }
    
    return NULL;
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
        case ERROR:
            execute_error_response(cmd, client);
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
        case RENAME:
            rename_exec(cmd, client, message_mutex, message_update_cond);
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

    setup_client(&client); // initializes client, sets connected=false, inits mutexes
    queue_init(&task_queue);
    
    //initialize socket without server and client authentication
    init_client_socket(&client);
    
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

    //spawn pre connection user input thread - active when not connected
    pthread_t user_input_pre_thread;
    user_input_args_t *user_input_pre_args = malloc(sizeof(user_input_args_t));
    setup_user_input_args(user_input_pre_args, &client);
    pthread_create(&user_input_pre_thread, NULL, user_input_pre_connection, user_input_pre_args);
    pthread_detach(user_input_pre_thread);

    //spawn post connection user input thread - active when connected
    pthread_t user_input_thread;
    user_input_args_t *user_input_args = malloc(sizeof(user_input_args_t));
    setup_user_input_args(user_input_args, &client);
    pthread_create(&user_input_thread, NULL, user_input, user_input_args);
    pthread_detach(user_input_thread);

    //spawn listener thread
    pthread_t listener_thread;
    cli_listener_args_t *cli_listener_args = malloc(sizeof(cli_listener_args_t));
    setup_cli_listner_args(cli_listener_args, &client, &task_queue);
    pthread_create(&listener_thread, NULL, cli_listener, cli_listener_args);
    pthread_detach(listener_thread);

    //spawn queue manager thread
    pthread_t queue_manager_thread;
    cli_queue_manager_args_t *cli_queue_manager_args = malloc(sizeof(cli_queue_manager_args_t));
    setup_cli_queue_manager_args(cli_queue_manager_args, &client, &task_queue, messages, &message_count, &messages_mutex, &messages_cond);
    pthread_create(&queue_manager_thread, NULL, cli_queue_manager, cli_queue_manager_args);
    pthread_detach(queue_manager_thread);
    

    pthread_exit(NULL); // exit main thread but keep other bg threads

    return 0;
}