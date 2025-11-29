#include "udp.h"
#include "queue.h"

//client
typedef struct {
    char name[NAME_SIZE];
    bool connected;

    int sd;
    struct sockaddr_in server_addr;
    struct sockaddr_in responder_addr;

} client_t;


void setup_client(client_t* client){
    client->sd = 0;
    client->connected = false;
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
    client->name = name;
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
} chat_display_args_t;

void setup_chat_display_args(chat_display_args_t* args, client_t * client, char messages[MAX_MSGS][MAX_LEN], int* message_count){
    args->client = client;
    args->messages = messages;
    args->message_count = message_count;
}

typedef struct {
    client_t * client;
    Queue * task_queue;
} cli_queue_manager_args_t;

void setup_cli_queue_manager_args(cli_listener_args_t* args, client_t * client, Queue* task_queue){
    args->client = client;
    args->task_queue = task_queue;
}

///cmds:///

//tbc... figure out what you need in here

//helper func join:
char* join(char arr[][50]) {
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

void say_exec(command_t* cmd, int* message_count, char messages[MAX_MSGS][MAX_LEN]){
    char * result = join(cmd->args);
    strcpy(messages[message_count], result);
    
}