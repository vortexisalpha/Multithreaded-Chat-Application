#include "cmd.h" // includes udp.h
#include "queue.h"


// command thread argument struct
typedef struct {
    int sd;
    command_t* command;
    struct sockaddr_in* from_addr;
    client_node_t *head;
    client_node_t *tail;
} execute_command_args_t;

void setup_command_args(execute_command_args_t* args, int sd, command_t* command, struct sockaddr_in* from_addr,client_node_t *head, client_node_t *tail){
    args->sd = sd;
    args->command = command;
    args->from_addr = from_addr;
    args->head = head;
    args->tail = tail;
}

// listner thread argument struct
typedef struct {
    int * sd;
    Queue * task_queue;
} listener_args_t;

void setup_listener_args(listener_args_t* args, int sd, Queue* q){
    args->sd = sd;
    args->task_queue = q;
}
// linked list node struct

typedef struct {
    client_node_t* next;
    struct sockaddr_in client_address;
    char client_name[NAME_SIZE];
} client_node_t;

void add_client_to_list(client_node_t * tail, struct sockaddr_in *addr, char name[NAME_SIZE]){
    client_node_t * new_node;
    strcpy(new_node->client_name, name);
    new_node->client_address = *addr;
    tail->next = new_node;
    tail = tail->next;
}

void remove_client_from_list(){}