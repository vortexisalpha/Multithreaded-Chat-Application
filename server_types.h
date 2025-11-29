#include "cmd.h" // includes udp.h
#include "queue.h"
#define MAX_MUTED 100
#define MAX_HISTORY 20 
#define MAX_LEN  256

#define BUFFER_SIZE 1024
#define SERVER_PORT 12000
#define NAME_SIZE 20

// linked list node struct

typedef struct client_node{
    struct client_node* next;
    struct sockaddr_in client_address;
    char client_name[NAME_SIZE];
} client_node_t;

void add_client_to_list(client_node_t **head,client_node_t **tail, struct sockaddr_in *addr, char name[NAME_SIZE]){
    client_node_t * new_node = malloc(sizeof(client_node_t));

    strncpy(new_node->client_name, name, NAME_SIZE);
    new_node->client_name[NAME_SIZE - 1] = '\0'; //strncpy is not always null terminated
    new_node->client_address = *addr;
    new_node->next = NULL;

    //if empty:
    if (*head == NULL){
        *head = new_node;
        *tail = new_node;
    } else{
        (*tail)->next = new_node;
        *tail = (*tail)->next;
    }
    
}


void remove_client_from_list(client_node_t *head, char name[NAME_SIZE]){
    client_node_t *iter = head; 
    client_node_t *last_node; 
    while(iter != NULL){
        if(strcmp(iter->client_name, name) == 0){
            printf("Remove client: %s\n", name);
            last_node->next = iter->next; 
            free(iter); 
            break;
        }
        last_node = iter; 
        iter = iter->next;
    }   

    if(iter == NULL){
        printf("Error. Client is not found in linked list\n");
    }

}

// command thread argument struct
typedef struct {
    int sd;
    command_t* command;
    struct sockaddr_in* from_addr;
    client_node_t **head;
    client_node_t **tail;
} execute_command_args_t;

void setup_command_args(execute_command_args_t* args, int sd, command_t* command, struct sockaddr_in* from_addr,client_node_t **head, client_node_t **tail){
    args->sd = sd;
    args->command = command;
    args->from_addr = from_addr;
    args->head = head;
    args->tail = tail;
}

// listner thread argument struct
typedef struct {
    int sd;
    Queue * task_queue;
} listener_args_t;

void setup_listener_args(listener_args_t* args, int sd, Queue* q){
    args->sd = sd;
    args->task_queue = q;
}

//queue manager thread argument struct
typedef struct {
    Queue * task_queue;
    int sd;
    client_node_t **head;
    client_node_t **tail;
} queue_manager_args_t;

void setup_queue_manager_args(queue_manager_args_t* args, Queue* q, int sd, client_node_t **head, client_node_t **tail){
    args->task_queue = q;
    args->sd = sd;
    args->head = head;
    args->tail = tail;
}

// muted chat buffer, local to each user
// initialise using new
typedef struct {
    char messages[MAX_MUTED][MAX_LEN];
    int count;
} muted_buffer_t;

// we need a chat history (slightly different from a queue) for (1) proposed extension (2) muted chat perservance
// this is a global struct (which I am not sure if the data structure is optimal)?
typedef struct {
    char messages[MAX_HISTORY][MAX_LEN];
    int count;
    pthread_mutex_t mutex;
} chat_history_t;

chat_history_t history;

///cmds:///

//tbc... figure out what you need in here
typedef struct {
    int sd;
    command_t* command;
    struct sockaddr_in* from_addr;
    client_node_t **head;
    client_node_t **tail;
} execute_command_args_t;

void setup_execute_command_args(execute_command_args_t * args){
    args->
}