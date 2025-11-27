#include "udp.h"
#include "queue.h"

//client
//i dont think we need a port
typedef struct {
    char name[NAME_SIZE];
    bool connected;

    int sd;
    struct sockaddr_in server_addr;
    struct sockaddr_in responder_addr;

} client_t;


void setup_client(client_t* client){
    printf("opening socket... \n");

    client->connected = true;
    client->sd = udp_socket_open(0);;

    printf("setting up port... \n");
    int rc = set_socket_addr(&client->server_addr, "127.0.0.1", SERVER_PORT);
    if (rc <= 0) printf("Client failed to connect to server");
}

//thread args:

typedef struct {
    client_t * client;
    Queue * task_queue;
} cli_listener_args_t;