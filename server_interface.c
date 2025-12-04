#include <stdlib.h>
#include "server_types.h"

// 1. The spawn and join should not be in the same thread - causing blocking -> not true multi-threading
//    Use a thread-pool configuration (which is the queue manager)
// 2. Use the name to be UID for now (assume no users with same name). Resolved by having login system (further enhancement)
// 3. Accessing shared resources solved by condition variable with reader/writer setup. 
// 4. SAY will spawns multiple threads, a 1-1 relation (server to client)
// 5. Mute & unmute lists will have a name list (pointer arrays). (update: client side responsibility)
// 6. 

// make this void *

// Discussion note with Josh:
// 1. The spawn and join should not be in the same thread - causing blocking -> not true multi-threading
//    Use a thread-pool configuration (which is the queue manager) (DONE, by having the detach)
// 2. Use the name to be UID for now (assume no users with same name). Resolved by having login system (further enhancement)
// 3. Accessing shared resources solved by condition variable with reader/writer setup.  (DONE, see Monitor_t)
// 4. SAY will spawns multiple threads, a 1-1 relation (server to client) (???)
// 5. Mute & unmute lists will have a name list (pointer arrays). 
// 6. Maximum worker control (DONE, see )



// client to server command interface
// send a string, separated by a space
// e.g., sayto 

void *listener(void *arg){
    listener_args_t* listener_args = (listener_args_t*)arg;
    //takes in the queue and adds the command into it,
    //queue is of form: [queue_node_t,...] each queue_node_t has msg and from addr
    printf("Server is listening on port %d\n", SERVER_PORT);
    while (1) 
    {
        char client_request[BUFFER_SIZE];

        struct sockaddr_in client_address;
    
        int rc = udp_socket_read(listener_args->sd, &client_address, client_request, BUFFER_SIZE);

        if (rc > 0){
            printf("[DEBUG] Server received: '%s' (%d bytes)\n", client_request, rc);
            q_append(listener_args->task_queue, client_request, &client_address); // this includes queue full sleep for thread
        } else if (rc < 0) {
            printf("[DEBUG] udp_socket_read error: %d\n", rc);
        }
    }
}


client_node_t* find_client_by_address(client_node_t** head, struct sockaddr_in* address){
    client_node_t *node = *head;
    while((node != NULL) && (memcmp(&(node->client_address), address, sizeof(struct sockaddr_in)) != 0)){
        node = node->next;
    }
    return node; 
}



void *connect_to_server(void* args){
    execute_command_args_t* cmd_args = (execute_command_args_t*)args; // cast to input type struct

    char* name = cmd_args->command->args[0];
    Monitor_t* client_linkedList = cmd_args->client_linkedList; 

    char ** chat_history = cmd_args->chat_history;
    int * chat_historyc = cmd_args->chat_historyc;

    // writer of linked list
    writer_checkin(client_linkedList);
    // enter critical section
    add_client_to_list(cmd_args->head, cmd_args->tail, cmd_args->from_addr, name);
    writer_checkout(client_linkedList);
    char server_response[BUFFER_SIZE];
    sprintf(server_response, "connsuccess$ %s", name);
    int rc = udp_socket_write(cmd_args->sd, cmd_args->from_addr, server_response, MAX_MESSAGE);
    printf("Request served: Connection successful for %s\n", name);

    char history_msg[RESPONSE_BUFFER_SIZE];
    int start = (*chat_historyc > CHAT_HISTORY_SIZE) ? *chat_historyc - CHAT_HISTORY_SIZE : 0;
    for (int i = start; i < *chat_historyc; i++){
        if (chat_history[i] != NULL){
            snprintf(history_msg, RESPONSE_BUFFER_SIZE, "say$ %s", chat_history[i]);
            rc = udp_socket_write(cmd_args->sd, cmd_args->from_addr, history_msg, RESPONSE_BUFFER_SIZE);
        }
    }

    printf("Sent chat history to: %s\n", name);
    //free heap allocated resources
    free(cmd_args->command);
    free(cmd_args->from_addr);
    free(args);
    return NULL;
}

client_node_t* find_client(client_node_t** head, char who[]){
    client_node_t* iter = *head; 
    while((iter != NULL) && (strcmp(iter->client_name, who) != 0) ){
        iter = iter->next; 
    }
    if(iter == NULL){
        printf("Error, client not found\n"); 
        return NULL;
    }
    return iter; 
}

void *sayto(void *args){
    // Example: SAYTO Alice Hello everyone how are you
    execute_command_args_t* cmd_args = (execute_command_args_t*)args;
    char* to_who = cmd_args->command->args[0]; 
    
    //join args starting from index 1
    char message[MAX_MESSAGE];
    join_args(cmd_args->command->args, 1, message, MAX_MESSAGE);
    
    client_node_t* client; 
    client_node_t* from_who; 
    Monitor_t* client_linkedList = cmd_args->client_linkedList; 

    // reader side handling
    reader_checkin(client_linkedList); 
    // enter critical section
    client = find_client(cmd_args->head, to_who); 
    from_who = find_client_by_address(cmd_args->head, cmd_args->from_addr); 
    reader_checkout(client_linkedList); 

    if (client != NULL){
        //send to reciever
        char server_reciever_response[RESPONSE_BUFFER_SIZE]; 
        snprintf(server_reciever_response, RESPONSE_BUFFER_SIZE, "say$ %s:(private message) %s", from_who->client_name, message); 
        int rc_r = udp_socket_write(cmd_args->sd, &(client->client_address), server_reciever_response, RESPONSE_BUFFER_SIZE);

        //send to client
        char server_sender_responce[RESPONSE_BUFFER_SIZE];
        snprintf(server_sender_responce, RESPONSE_BUFFER_SIZE, "say$ you to %s:(private message) %s", to_who, message);
        int rc_s = udp_socket_write(cmd_args->sd, &(from_who->client_address), server_sender_responce, RESPONSE_BUFFER_SIZE);
    } else{
        //send to client
        char server_sender_responce[RESPONSE_BUFFER_SIZE];
        snprintf(server_sender_responce, RESPONSE_BUFFER_SIZE, "error$ User not found. (%s)", to_who);
        int rc_s = udp_socket_write(cmd_args->sd, &(from_who->client_address), server_sender_responce, RESPONSE_BUFFER_SIZE);
    }


    //free heap allocated resources
    free(cmd_args->command);
    free(cmd_args->from_addr);
    free(cmd_args);
    return NULL;
}

//sayto:
//snprintf(server_response, MAX_MESSAGE, "say$ %s:(private) %s", from_who->client_name, message); 
//say:
//snprintf(server_response, MAX_MESSAGE, "say$ %s: %s", from_who->client_name, message); 


void *say(void *args){
    // Example: SAY Hello everyone!
    execute_command_args_t* cmd_args = (execute_command_args_t*)args;
    
    //join all arguments into full message
    char full_message[MAX_MESSAGE + MAX_NAME_LEN + 2];
    char message[MAX_MESSAGE];
    join_args(cmd_args->command->args, 0, message, MAX_MESSAGE);
    
    char ** chat_history = cmd_args->chat_history;
    int *chat_historyc = cmd_args->chat_historyc;
    
    client_node_t* node = *(cmd_args->head); 
    client_node_t* from_who; 
    reader_checkin(cmd_args->client_linkedList);
    from_who = find_client_by_address(cmd_args->head, cmd_args->from_addr); 
    reader_checkout(cmd_args->client_linkedList); 

    snprintf(full_message, MAX_MESSAGE + MAX_NAME_LEN + 2, "%s: %s", from_who->client_name, message);
    char server_response[RESPONSE_BUFFER_SIZE];
    snprintf(server_response, RESPONSE_BUFFER_SIZE, "say$ %s", full_message);


    if (*chat_historyc < MAX_MSGS){
        chat_history[*chat_historyc] = strdup(full_message);
        (*chat_historyc)++;
    }

    reader_checkin(cmd_args->client_linkedList); 
    while(node != NULL){ 
        int rc = udp_socket_write(cmd_args->sd, &node->client_address, server_response, RESPONSE_BUFFER_SIZE); 
        node = node->next; 
    }
    reader_checkout(cmd_args->client_linkedList);
    
    free(cmd_args->command);
    free(cmd_args->from_addr);
    free(cmd_args);
    return NULL;
    /*
    char* message = cmd_args->command->args[1];     
    client_node_t *iter = cmd_args->head; 
    client_node_t *check = cmd_args->tail; 
    while((iter!=check)&&(iter!=NULL)){
        char* send_message[MAX_MESSAGE]; 
        strncat(send_message, who, sizeof(send_message) - strlen(send_message)-1); 
        strncat(send_message, ": ", sizeof(send_message) - strlen(send_message)-1); 
        strncat(send_message, message, sizeof(send_message) - strlen(send_message)-1);
        int rc = udp_socket_write(cmd_args->sd, &(iter->client_address), send_message, strlen(send_message)+1);
        iter = iter->next; 
    }
    */ 
}

void *disconnect(void *args){
    execute_command_args_t* cmd_args = (execute_command_args_t*)args; // cast to input type struct
    client_node_t* node;
    char* name; 
    struct sockaddr_in* client_address; 
    
    //find client by their address
    reader_checkin(cmd_args->client_linkedList); 
    node = find_client_by_address(cmd_args->head, cmd_args->from_addr); 
    if(node == NULL){
        reader_checkout(cmd_args->client_linkedList);
        free(cmd_args->command);
        free(cmd_args->from_addr);
        free(args);
        return NULL;
    }
    name = node->client_name; 
    reader_checkout(cmd_args->client_linkedList); 
    
    //remove client from list
    writer_checkin(cmd_args->client_linkedList); 
    // enter critical section
    client_address = remove_client_from_list(cmd_args->head, cmd_args->tail, name); 
    if(client_address != NULL){
        writer_checkout(cmd_args->client_linkedList); 

        // server response
        char server_response[MAX_MESSAGE]; 
        snprintf(server_response, MAX_MESSAGE, "disconnresponse$"); 
        int rc = udp_socket_write(cmd_args->sd, client_address, server_response, MAX_MESSAGE); 
        free(client_address);
    } else {
        writer_checkout(cmd_args->client_linkedList);
    }
    
    free(cmd_args->command);
    free(cmd_args->from_addr);
    free(args);
    return NULL;
}


void *mute(void *args){
    execute_command_args_t* cmd_args = (execute_command_args_t*)args; // cast to input type struct
    char* username = cmd_args->command->args[0];
    
    //send mute command back to client so they can add to their mute_table
    char server_response[MAX_MESSAGE];
    snprintf(server_response, MAX_MESSAGE, "mute$ %s", username);
    udp_socket_write(cmd_args->sd, cmd_args->from_addr, server_response, MAX_MESSAGE);
    
    free(cmd_args->command);
    free(cmd_args->from_addr);
    free(cmd_args);
    return NULL;
}

void *unmute(void *args){
    execute_command_args_t* cmd_args = (execute_command_args_t*)args; // cast to input type struct
    char* username = cmd_args->command->args[0];
    
    //send unmute command back to client so they can remove from their mute_table
    char server_response[MAX_MESSAGE];
    snprintf(server_response, MAX_MESSAGE, "unmute$ %s", username);
    udp_socket_write(cmd_args->sd, cmd_args->from_addr, server_response, MAX_MESSAGE);
    
    free(cmd_args->command);
    free(cmd_args->from_addr);
    free(cmd_args);
    return NULL;
}


void *rename_client(void *args){
    printf("[DEBUG] Rename hit\n");

    execute_command_args_t* cmd_args = (execute_command_args_t*)args; // cast to input type struct
    struct sockaddr_in* cli_to_rename = cmd_args->from_addr;
    char* new_name = cmd_args->command->args[0]; 
    client_node_t *client; 

    writer_checkin(cmd_args->client_linkedList); 
    // critical section
    client = find_client_by_address(cmd_args->head, cli_to_rename); 
    printf("[DEBUG] Renaming %s to %s\n", client->client_name, new_name);

    strcpy(client->client_name, new_name); 

    writer_checkout(cmd_args->client_linkedList);

    char server_response[MAX_MESSAGE]; 
    snprintf(server_response, MAX_MESSAGE, "rename$ %s", new_name); 
    int rc = udp_socket_write(cmd_args->sd, cli_to_rename, server_response, MAX_MESSAGE);

    
    free(cmd_args->command);
    free(cmd_args->from_addr);
    free(cmd_args);
    return NULL;
}


void *kick(void *args){
    execute_command_args_t* cmd_args = (execute_command_args_t*)args; // cast to input type struct
    // remove the client from the linked list
    char* name = cmd_args->command->args[0]; 
    struct sockaddr_in* client_address; 
    writer_checkin(cmd_args->client_linkedList); 
    // enter critical section
    client_address = remove_client_from_list(cmd_args->head, cmd_args->tail, name); 
    writer_checkout(cmd_args->client_linkedList); 

    if(client_address != NULL){
        // send reciever
        char server_reciever_response[MAX_MESSAGE]; 
        snprintf(server_reciever_response, MAX_MESSAGE, "disconnresponse$"); 
        int rc = udp_socket_write(cmd_args->sd, client_address, server_reciever_response, MAX_MESSAGE);
        
        // send everyone "(Whom) has been removed from the chat"
        char broadcast_message[RESPONSE_BUFFER_SIZE];
        snprintf(broadcast_message, RESPONSE_BUFFER_SIZE, "say$ SERVER: %s has been kicked", name);
        
        reader_checkin(cmd_args->client_linkedList);
        client_node_t *node = *(cmd_args->head);
        while(node != NULL){
            udp_socket_write(cmd_args->sd, &node->client_address, broadcast_message, RESPONSE_BUFFER_SIZE);
            node = node->next;
        }
        reader_checkout(cmd_args->client_linkedList);
    }
    free(client_address);
    free(cmd_args->command);
    free(cmd_args->from_addr);
    free(args);
    return NULL;
}

void *handle_unknown_command(void *args){
    execute_command_args_t* cmd_args = (execute_command_args_t*)args;
    
    //send error message back to client
    char server_response[MAX_MESSAGE]; 
    snprintf(server_response, MAX_MESSAGE, "error$ Unknown command"); 
    int rc = udp_socket_write(cmd_args->sd, cmd_args->from_addr, server_response, MAX_MESSAGE);
    
    printf("Invalid command received from client\n");
    
    //free
    free(cmd_args->command);
    free(cmd_args->from_addr);
    free(args);
    return NULL;
}



void spawn_execute_command_threads(int sd, command_t* command, struct sockaddr_in* from_addr, client_node_t **head, client_node_t **tail, Monitor_t* client_linkedList, char** chat_history, int* chat_historyc){
    //make all threads neccesary for command to be executed and wait for them to be executed
    pthread_t t;
    execute_command_args_t *execute_args = malloc(sizeof(execute_command_args_t));
    setup_command_args(execute_args, sd, command, from_addr, head, tail, client_linkedList, chat_history, chat_historyc);
    
    //if the connection is anything but CONN we mark it as the client behaving
    if (command->kind != CONN) {
        update_client_activity(head, from_addr);
    }
    
    printf("[DEBUG] Found command kind: %d\n", command->kind);
    switch(command->kind){
        case CONN:{
            pthread_create(&t, NULL, connect_to_server, execute_args);
            break;
        }
        case SAY:
            pthread_create(&t, NULL, say, execute_args); 
            break; 
        case SAYTO:
            pthread_create(&t, NULL, sayto, execute_args); 
            break; 
        case DISCONN:
            pthread_create(&t, NULL, disconnect, execute_args); 
            break; 
        case MUTE:
            pthread_create(&t, NULL, mute, execute_args); 
            break; 
        case UNMUTE:
            pthread_create(&t, NULL, unmute, execute_args); 
            break; 
        case RENAME:
            pthread_create(&t, NULL, rename_client, execute_args); 
            break; 
        case KICK:
            pthread_create(&t, NULL, kick, execute_args); 
            break;
        case RETPING:
            // if retping command ignore it
            free(command);
            free(from_addr);
            free(execute_args);
            return; 
        case UNKNOWN:
            pthread_create(&t, NULL, handle_unknown_command, execute_args);
            break;
        default:
            free(command);
            free(from_addr);
            free(execute_args);
            return;
    }
    pthread_detach(t); 

}

void *queue_manager(void* arg){
    queue_manager_args_t* qm_args = (queue_manager_args_t *)arg;

    //initialise chat history array (pointer to first element of pointer array)
    char ** chat_history = calloc(MAX_MSGS, sizeof(char*));    
    int chat_historyc = 0;

    while(1){
        char * tokenised_command[MAX_COMMAND_LEN];
       
        struct sockaddr_in* from_addr = malloc(sizeof(struct sockaddr_in));
        q_pop(qm_args->task_queue, tokenised_command, from_addr);
       
        command_t* cur_command = malloc(sizeof(command_t));
        command_handler(cur_command, tokenised_command);
        worker_thread_checkin(qm_args); 
        spawn_execute_command_threads(qm_args->sd, cur_command, from_addr, qm_args->head, qm_args->tail, qm_args->client_linkedList, chat_history, &chat_historyc);
        worker_thread_checkout(qm_args); 
    }
}

void* connection_manager(void* arg){
    connection_manager_args_t* cm_args = (connection_manager_args_t *)arg;

    printf("[DEBUG] Started connection manager thread\n");
    
    while(1){
        sleep(1);
        
        time_t current_time = time(NULL);
        client_node_t *node;
        client_node_t *to_ping = NULL;
        client_node_t *to_kick = NULL;
        char kick_name[NAME_SIZE];
        
        //Like a water overflowing algorithm:
        //node "pours" into to_ping if its last command was exucuted over the threshold of seconds ago and the node is sent a ping request
        //to_ping "pours" into to_kick if it does not respond to the request
        reader_checkin(cm_args->client_linkedList);
        node = *cm_args->head;
        while(node != NULL){ 
            double inactive_seconds = difftime(current_time, node->last_active);
            
            if (inactive_seconds > INACTIVITY_THRESHOLD) {
                if (node->ping_sent == 0) {
                    to_ping = node;
                    break;
                } else {
                    double ping_wait = difftime(current_time, node->ping_sent);
                    if (ping_wait > PING_TIMEOUT) {
                        // no response from ping so kick client
                        strncpy(kick_name, node->client_name, NAME_SIZE);
                        kick_name[NAME_SIZE - 1] = '\0';
                        to_kick = node;
                        printf("[DEBUG] Client '%s' didn't respond to ping (waited %.0fs)\n", kick_name, ping_wait);
                        break;
                    }
                }
            }
            node = node->next; 
        }
        reader_checkout(cm_args->client_linkedList);
        
        //send ping
        if (to_ping != NULL) {
            char ping_msg[MAX_MESSAGE];
            snprintf(ping_msg, MAX_MESSAGE, "ping$");
            udp_socket_write(cm_args->sd, &to_ping->client_address, ping_msg, MAX_MESSAGE);
            to_ping->ping_sent = current_time;
            printf("[DEBUG] Sent ping to '%s'\n", to_ping->client_name);
        }
        
        //kick client when not responded to ping
        if (to_kick != NULL) {
            //disconnect to_kick
            char server_response[MAX_MESSAGE];
            snprintf(server_response, MAX_MESSAGE, "disconnresponse$");
            udp_socket_write(cm_args->sd, &to_kick->client_address, server_response, MAX_MESSAGE);
            
            //remove from linked list
            writer_checkin(cm_args->client_linkedList); 
            struct sockaddr_in* removed_addr = remove_client_from_list(cm_args->head, cm_args->tail, kick_name);
            writer_checkout(cm_args->client_linkedList);
            
            free(removed_addr);
            printf("Client (%s) kicked for inactivity\n", kick_name);
        }
    }
    return NULL;
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

    // init a share state (for linked list)
    Monitor_t* client_linkedList = malloc(sizeof(Monitor_t)); 
    monitor_init(client_linkedList); 
    
    //spawn listner
    pthread_t listener_thread;
    listener_args_t *listener_args = malloc(sizeof(listener_args_t));
    setup_listener_args(listener_args, sd, &task_queue);
    pthread_create(&listener_thread, NULL, listener, listener_args);
    pthread_detach(listener_thread); //?

    //spawn queue manager thread
    // requires synchronisation here (Linked List & chat history)
    pthread_t queue_manager_thread;
    queue_manager_args_t *queue_manager_args = malloc(sizeof(queue_manager_args_t));
    setup_queue_manager_args(queue_manager_args,&task_queue, sd, &head, &tail, client_linkedList);
    pthread_create(&queue_manager_thread, NULL, queue_manager, queue_manager_args);
    pthread_detach(queue_manager_thread);

    pthread_t connection_manager_thread;
    connection_manager_args_t *connection_manager_args = malloc(sizeof(connection_manager_args_t));
    setup_connection_manager_args(connection_manager_args, sd, &head, &tail, client_linkedList);
    pthread_create(&connection_manager_thread, NULL, connection_manager, connection_manager_args);
    pthread_detach(connection_manager_thread);

   
    pthread_exit(NULL); // exit main thread but keep other bg threads
    return 0;
}