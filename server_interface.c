
#include <stdio.h>
#include <stdlib.h>
#include "server_types.h"

// make this void *

void *listener(void *arg){
    listener_args_t* listener_args = (listener_args_t*)arg;
    //takes in the queue and adds the command into it,
    //queue is of form: [queue_node_t,...] each queue_node_t has msg and from addr
    printf("Server is listening on port %d\n", SERVER_PORT);
    while (1) 
    {
        // Storage for request and response messages
        char client_request[BUFFER_SIZE];

        // Variable to store incoming client's IP address and port
        struct sockaddr_in client_address;
    
        int rc = udp_socket_read(listener_args->sd, &client_address, client_request, BUFFER_SIZE);

        if (rc > 0){
            q_append(listener_args->task_queue, client_request, client_address); // this includes queue full sleep for thread
        }
    }
}


void *connect_to_server(void* args){
    execute_command_args_t* cmd_args = (execute_command_args_t*)args; // cast to input type struct
    char* name = cmd_args->command->args[0];
    add_client_to_list(cmd_args->head, cmd_args->tail, cmd_args->from_addr, name);

    char server_response[BUFFER_SIZE];
    sprintf(server_response, "CONNECTED %s", name);
    int rc = udp_socket_write(cmd_args->sd, cmd_args->from_addr, server_response, strlen(server_response) + 1);
    printf("Request served...\n");

    free(args);
    return NULL;
}


void spawn_execute_command_threads(int sd, command_t* command, struct sockaddr_in* from_addr, client_node_t **head, client_node_t **tail){
    //make all threads neccesary for command to be executed and wait for them to be executed
    switch(command->kind){
        case CONN:{
            pthread_t t;
            execute_command_args_t *execute_args = malloc(sizeof(execute_command_args_t));
            setup_command_args(execute_args,sd,command, from_addr, head, tail);
            pthread_create(&t, NULL, connect_to_server, execute_args);
            pthread_join(t, NULL); //?
            break;
        }
        default:
            break;
    }
}

void *queue_manager(void* arg){
    queue_manager_args_t* qm_args = (queue_manager_args_t *)arg;

    while(1){
        char * tokenised_command[MAX_COMMAND_LEN];
        struct sockaddr_in from_addr;
        q_pop(qm_args->task_queue, tokenised_command, &from_addr); // pop command from front of command queue (includes sleep wait for queue nonempty)

        command_t cur_command;
        command_handler(&cur_command, tokenised_command); // fill out cur_command

        spawn_execute_command_threads(qm_args->sd, &cur_command, &from_addr, qm_args->head, qm_args->tail);
        
    }
}

void setup(int* sd, Queue * q){
    *sd = udp_socket_open(SERVER_PORT);
    assert(*sd > -1);

    queue_init(q);
}


int main(int argc, char *argv[])
{

    // This function opens a UDP socket,
    // binding it to all IP interfaces of this machine,
    // and port number SERVER_PORT
    // (See details of the function in udp.h)
    int sd;
    client_node_t *head = NULL;
    client_node_t *tail = NULL;
    Queue task_queue;
    setup(&sd, &task_queue);
    
    //spawn listner
    pthread_t listener_thread;
    listener_args_t *listener_args = malloc(sizeof(listener_args_t));
    setup_listener_args(listener_args, sd, &task_queue);
    pthread_create(&listener_thread, NULL, listener, listener_args);
    pthread_detach(listener_thread); //?

    //spawn queue manager thread
    pthread_t queue_manager_thread;
    queue_manager_args_t *queue_manager_args = malloc(sizeof(queue_manager_args_t));
    setup_queue_manager_args(queue_manager_args,&task_queue, sd, &head, &tail);
    pthread_create(&queue_manager_thread, NULL, queue_manager, queue_manager_args);
    pthread_detach(queue_manager_thread);


    pthread_exit(NULL); // exit main thread but keep other bg threads
    return 0;
}