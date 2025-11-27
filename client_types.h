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


typedef struct{ 
    client_t * client;
    messages[MAX_MSGS][MAX_LEN];
    int * message_count;
} chat_display_args_t;
