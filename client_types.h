#include "udp.h"
#include "queue.h"
#include "cmd.h"
#include "custom_hash_table.h"


//client
typedef struct {
    char name[NAME_SIZE];
    bool connected;
    int sd;
    struct sockaddr_in server_addr;
    struct sockaddr_in responder_addr;
    hash_node_t *mute_table[TABLE_SIZE];
    
    //mutex for if client is connected
    pthread_mutex_t connection_mutex;
    pthread_cond_t connection_cond;
} client_t;


void setup_client(client_t* client){
    client->sd = 0;
    client->connected = false;
    for (int i = 0; i < TABLE_SIZE; i++) {
        client->mute_table[i] = NULL;
    }
    
    pthread_mutex_init(&client->connection_mutex, NULL);
    pthread_cond_init(&client->connection_cond, NULL);
}

void connect_command(client_t* client, char name[NAME_SIZE]){
    printf("opening socket... \n");
    client->sd = udp_socket_open(0);;
    
    printf("Setting up port... \n");
    int rc = set_socket_addr(&client->server_addr, "127.0.0.1", SERVER_PORT);
    if (rc <= 0) {
        printf("Client failed to connect to server");
        return;
    }
    strcpy(client->name, name);
    client->connected = true;
}

//thread args:

typedef struct {
    client_t * client;
    Queue * task_queue;
} cli_listener_args_t;


void setup_cli_listner_args(cli_listener_args_t* args, client_t * client, Queue* task_queue){
    args->client = client;
    args->task_queue = task_queue;
}

typedef struct{ 
    client_t * client;
    char (*messages)[MAX_LEN];
    int * message_count;
    pthread_mutex_t * messages_mutex;
    pthread_cond_t * messages_cond;
} chat_display_args_t;

void setup_chat_display_args(chat_display_args_t* args, client_t * client, char (*messages)[MAX_LEN], int* message_count, pthread_mutex_t* message_mutex, pthread_cond_t* message_update_cond){
    args->client = client;
    args->messages = messages;
    args->message_count = message_count;
    args->messages_mutex = message_mutex;
    args->messages_cond = message_update_cond;
}

typedef struct {
    client_t * client;
} user_input_args_t;

void setup_user_input_args(user_input_args_t* args, client_t * client){
    args->client = client;
}

typedef struct {
    client_t * client;
    Queue * task_queue;
    char (*messages)[MAX_LEN];
    int * message_count;
    pthread_mutex_t * messages_mutex;
    pthread_cond_t * messages_cond;
} cli_queue_manager_args_t;

void setup_cli_queue_manager_args(cli_queue_manager_args_t* args, client_t * client, Queue* task_queue, char (*messages)[MAX_LEN], int* message_count, pthread_mutex_t* message_mutex, pthread_cond_t* message_update_cond){
    args->client = client;
    args->task_queue = task_queue;
    args->messages = messages;
    args->message_count = message_count;
    args->messages_mutex = message_mutex;
    args->messages_cond = message_update_cond;
}

typedef struct {
    client_t * client;
    char (*messages)[MAX_LEN];
    int * message_count;
    pthread_mutex_t * messages_mutex;
    pthread_cond_t * messages_cond;
    char ** tokenised_command;  // pointer to dynamically allocated array
} handle_cli_side_cmd_args_t;

void setup_handle_cli_side_cmd_args(handle_cli_side_cmd_args_t* args, client_t * client, char (*messages)[MAX_LEN], int * message_count, pthread_mutex_t * messages_mutex, pthread_cond_t * messages_cond, char ** tokenised_command){
    args->client = client;
    args->messages = messages;
    args->message_count = message_count;
    args->messages_mutex = messages_mutex;
    args->messages_cond = messages_cond;  // Fixed: was message_update_cond
    args->tokenised_command = tokenised_command;
}
///cmds:///

//tbc... figure out what you need in here

//helper func join:
char* join(char arr[][NAME_SIZE]) {
    static char result[300];
    result[0] = '\0';

    int i = 0;
    while (arr[i][0] != '\0') {
        strcat(result, arr[i]);

        if (arr[i + 1][0] != '\0') 
            strcat(result, " ");

        i++;
    }

    return result;
}

void say_exec(command_t* cmd, int* message_count, char (*messages)[MAX_LEN], pthread_mutex_t* message_mutex, pthread_cond_t* message_update_cond){
    pthread_mutex_lock(message_mutex);
    char * result = join(cmd->args);
    strcpy(messages[*message_count], result);
    (*message_count)++;
    pthread_cond_signal(message_update_cond);  //signal that messages were updated
    pthread_mutex_unlock(message_mutex);
}

void mute_exec(command_t* cmd, client_t * client, pthread_mutex_t* message_mutex, pthread_cond_t* message_update_cond){
    pthread_mutex_lock(message_mutex);

    char* username = cmd->args[0];
    insert(client->mute_table, username);

    pthread_cond_signal(message_update_cond);
    pthread_mutex_unlock(message_mutex);
}

void unmute_exec(command_t* cmd, client_t * client, pthread_mutex_t* message_mutex, pthread_cond_t* message_update_cond){
    pthread_mutex_lock(message_mutex);

    const char* username = cmd->args[0];
    remove_key(client->mute_table, username);

    pthread_cond_signal(message_update_cond);
    pthread_mutex_unlock(message_mutex);
}


//handle connection success
void execute_connect_response(command_t *cmd, client_t * client, pthread_mutex_t* message_mutex, pthread_cond_t* message_update_cond){
    char* username = cmd->args[0];
    
    pthread_mutex_lock(&client->connection_mutex);

    client->connected = true;
    strcpy(client->name, username);
    
    pthread_cond_signal(&client->connection_cond); // signal connected to wake up true listner thread
    pthread_mutex_unlock(&client->connection_mutex);
    
    // Trigger chat display update to show new prompt
    pthread_mutex_lock(message_mutex);
    pthread_cond_signal(message_update_cond);
    pthread_mutex_unlock(message_mutex);
    
    printf("Successfully connected as: %s\n", username);
}

//handle connection failure
void execute_connect_failed(command_t *cmd, client_t * client){
    char* error_message = join(cmd->args);
    printf("Connection failed: %s\n", error_message);
    printf("Please try again with a different username.\n");
}

//handle error from server
void execute_error_response(command_t *cmd, client_t * client){
    char* error_message = join(cmd->args);
    printf("Error from server: %s\n", error_message);
    
    pthread_mutex_lock(&client->connection_mutex);
    if (client->connected) {
        printf("[%s] > ", client->name);
    } else {
        printf("[Not connected] > ");
    }
    pthread_mutex_unlock(&client->connection_mutex);
    fflush(stdout);
}

//handle disconnect
void execute_disconnect_response(client_t * client, int * message_count, pthread_mutex_t* message_mutex, pthread_cond_t* message_update_cond){
    pthread_mutex_lock(&client->connection_mutex);
    
    //set connection state to false and close socket
    client->connected = false;
    /*
    if (client->sd > 0) {
        close(client->sd);
        client->sd = 0;
    }
    */
    
    //wake up the pre connection input thread
    pthread_cond_signal(&client->connection_cond);
    pthread_mutex_unlock(&client->connection_mutex);
    
    //clear chat history
    pthread_mutex_lock(message_mutex);
    *message_count = 0;
    pthread_cond_signal(message_update_cond);
    pthread_mutex_unlock(message_mutex);
    
    printf("Disconnected from server.\n");
}
