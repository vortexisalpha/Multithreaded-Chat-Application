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
} client_t;


void setup_client(client_t* client){
    client->sd = 0;
    client->connected = false;
    for (int i = 0; i < TABLE_SIZE; i++) {
        client->mute_table[i] = NULL;
    }
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