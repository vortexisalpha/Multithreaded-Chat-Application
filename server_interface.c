
#include <stdio.h>
#include <stdlib.h>
#include "udp.h"
#include "queue.h"

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

void *listener(void *queue, int * sd, Queue * task_queue){
    //takes in the queue and adds the command into it in the form:
    //[]
    while (1) 
    {
        // Storage for request and response messages
        char client_request[BUFFER_SIZE];

        // Demo code (remove later)
        printf("Server is listening on port %d\n", SERVER_PORT);

        // Variable to store incoming client's IP address and port
        struct sockaddr_in client_address;
    
        int rc = udp_socket_read(&sd, &client_address, client_request, BUFFER_SIZE);

        if (rc > 0){
            q_append(task_queue, client_request, client_address); // this includes queue full sleep for thread
        }
    }
}

typedef struct {
    int sd;
    command_t* command;
    struct sockaddr_in* from_addr;
    client_node_t *head;
    client_node_t *tail;
} execute_command_args_t;

setup_command_args(execute_command_args_t* args, int sd, command_t* command, struct sockaddr_in* from_addr,client_node_t *head, client_node_t *tail){
    args->sd = sd;
    args->command = command;
    args->from_addr = from_addr;
    args->head = head;
    args->tail = tail;
}

void *connect_to_server(void* arg){
    execute_command_args_t* cmd_args = (execute_command_args_t*)arg; // cast to input type struct
    char* name = cmd_args->command->args[0];
    add_client_to_list(cmd_args->tail, cmd_args->from_addr, name);

    char server_response[BUFFER_SIZE];
    sprintf(server_response, "CONNECTED %s", name);
    int rc = udp_socket_write(cmd_args->sd, &cmd_args->from_addr, server_response, BUFFER_SIZE);
}


void spawn_execute_command_threads(int sd, command_t* command, struct sockaddr_in* from_addr, client_node_t *head, client_node_t *tail){
    //make all threads neccesary for command to be executed and wait for them to be executed
    switch(command->kind){
        case CONN:
            pthread_t t;
            execute_command_args_t *execute_args;
            setup_command_args(execute_args,sd,command, from_addr, head, tail);
            pthread_create(&t, NULL, connect_to_server, execute_args);
            pthread_join(t, NULL); //?
            break;
    }
}

void *queue_thread(Queue * task_queue, int sd, client_node_t *head, client_node_t *tail){
    while(1){
        char * tokenised_command[MAX_COMMAND_LEN];
        struct sockaddr_in from_addr;
        q_pop(task_queue, tokenised_command, &from_addr); // pop command from front of command queue (includes sleep wait for queue nonempty)

        command_t cur_command;
        command_handler(&cur_command, tokenised_command); // fill out cur_command

        spawn_execute_command_threads(sd, &cur_command, &from_addr, head, tail);
        
        printf("Request served...\n");
    }
}

void setup(int* sd, Queue * q){
    *sd = udp_socket_open(SERVER_PORT);
    assert(sd > -1);

    queue_init(q);
}


int main(int argc, char *argv[])
{

    // This function opens a UDP socket,
    // binding it to all IP interfaces of this machine,
    // and port number SERVER_PORT
    // (See details of the function in udp.h)
    int sd;
    client_node_t *head;
    client_node_t *tail = head;
    Queue task_queue;
    setup(sd, &task_queue);
    
    // Server main loop
    while (1) 
    {
        // Storage for request and response messages
        char client_request[BUFFER_SIZE], server_response[BUFFER_SIZE];

        // Demo code (remove later)
        printf("Server is listening on port %d\n", SERVER_PORT);

        // Variable to store incoming client's IP address and port
        struct sockaddr_in client_address;
    
        int rc = udp_socket_read(sd, &client_address, client_request, BUFFER_SIZE);

        // Successfully received an incoming request
        if (rc > 0)
        {
            // Demo code (remove later)
            strcpy(server_response, "Hi, the server has received: ");
            strcat(server_response, client_request);
            strcat(server_response, "\n");
            char* port_n;
            sprintf(port_n, "client port: %d", ntohs(client_address.sin_port));
            strcat(server_response, port_n);

            rc = udp_socket_write(sd, &client_address, server_response, BUFFER_SIZE);

            printf("Request served...\n");
        }
    }

    return 0;
}