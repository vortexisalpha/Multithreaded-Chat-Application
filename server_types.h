#include "cmd.h" // includes udp.h
#include "queue.h"
#include <time.h>

#define MAX_NAMES 50
#define MAX_NAME_LEN 30
#define MAX_THREADS 128
#define CHAT_HISTORY_SIZE 15
#define INACTIVITY_THRESHOLD 180 //3mins
#define PING_TIMEOUT 10

// Note: MAX_LEN, BUFFER_SIZE, SERVER_PORT defined in udp.h

//helper function to join command arguments with spaces starting from start_index
void join_args(char args[][NAME_SIZE], int start_index, char* result, int max_len) {
    result[0] = '\0';
    int current_len = 0;
    bool first = true;
    
    for (int i = start_index; i < MAX_CMD_SIZE && args[i][0] != '\0'; i++) {
        //add space before arg except first
        if (!first && current_len < max_len - 1) {
            strcat(result, " ");
            current_len++;
        }
        
        int arg_len = strlen(args[i]);
        if (current_len + arg_len < max_len - 1) {
            strcat(result, args[i]);
            current_len += arg_len;
            first = false;
        } else {
            break; //buffer full
        }
    }
}

// linked list node struct

typedef struct client_node{
    struct client_node* next;
    struct sockaddr_in client_address;
    char client_name[NAME_SIZE];
    time_t last_active;
    time_t ping_sent; //time when ping was sent
} client_node_t;


typedef struct{
    int AR; // active reader
    int WR; // waiting reader

    int AW; // active writer
    int WW; // waiting writer

    pthread_mutex_t lock;
    pthread_cond_t cond_read; 
    pthread_cond_t cond_write;
    client_node_t *client_head;
} Monitor_t; 

void add_client_to_list(client_node_t **head,client_node_t **tail, struct sockaddr_in *addr, char name[NAME_SIZE]){
    client_node_t * new_node = malloc(sizeof(client_node_t));

    strncpy(new_node->client_name, name, NAME_SIZE);
    new_node->client_name[NAME_SIZE - 1] = '\0'; //strncpy is not always null terminated
    new_node->client_address = *addr;
    new_node->next = NULL;
    new_node->last_active = time(NULL);
    new_node->ping_sent = 0;

    //if empty:
    if (*head == NULL){
        *head = new_node;
        *tail = new_node;
    } else{
        (*tail)->next = new_node;
        *tail = (*tail)->next;
    }
    
}

//update last_active time for a client
void update_client_activity(client_node_t **head, struct sockaddr_in *addr) {
    client_node_t *node = *head;
    while (node != NULL) {
        if (memcmp(&(node->client_address), addr, sizeof(struct sockaddr_in)) == 0) {
            node->last_active = time(NULL);
            node->ping_sent = 0;
            return;
        }
        node = node->next;
    }
}

void monitor_init(Monitor_t* monitor){
    monitor->AR=0; // active reader
    monitor->WR=0; // waiting reader

    monitor->AW=0; // active writer
    monitor->WW=0; // waiting writer

    pthread_mutex_init(&monitor->lock, NULL);
    pthread_cond_init(&monitor->cond_read, NULL); 
    pthread_cond_init(&monitor->cond_write, NULL); 
    monitor->client_head = NULL;
}


struct sockaddr_in* remove_client_from_list(client_node_t **head, client_node_t **tail, char name[NAME_SIZE]){
    client_node_t *iter = *head; 
    client_node_t *last_node = NULL; 
    struct sockaddr_in* client_address = NULL; 
    
    while(iter != NULL){
        if(strcmp(iter->client_name, name) == 0){
            printf("Remove client: %s\n", name);

            client_address = malloc(sizeof(struct sockaddr_in));
            *client_address = iter->client_address;
            
            //update head if removing first node
            if(last_node == NULL){
                *head = iter->next;
            } else {
                last_node->next = iter->next;
            }
            
            //update tail if removing last node
            if(iter == *tail){
                *tail = last_node;
            }
            
            free(iter);
            return client_address; 
        }
        last_node = iter; 
        iter = iter->next;
    }
    
    printf("Error. Client is not found in linked list\n");
    return NULL; 
}

// command thread argument struct
typedef struct {
    int sd;
    command_t* command;
    struct sockaddr_in* from_addr;
    client_node_t **head;
    client_node_t **tail;
    Monitor_t* client_linkedList; 
    char** chat_history;
    int* chat_historyc;
} execute_command_args_t;

void setup_command_args(execute_command_args_t* args, int sd, command_t* command, struct sockaddr_in* from_addr,client_node_t **head, client_node_t **tail, Monitor_t* client_linkedList, char ** chat_history, int *chat_historyc){
    args->sd = sd;
    args->command = command;
    args->from_addr = from_addr;
    args->head = head;
    args->tail = tail;
    args->client_linkedList = client_linkedList; 
    args->chat_history = chat_history;  
    args->chat_historyc = chat_historyc;  
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
//queue manager is essentially a thread-pool configuration
typedef struct {
    Queue * task_queue;
    int sd;
    client_node_t **head;
    client_node_t **tail;
    Monitor_t* client_linkedList; 

    // add worker constraints
    // worker constraints
    int active_worker; 
    pthread_cond_t cond_worker; 
    pthread_mutex_t pool_lock; 
} queue_manager_args_t;

void setup_queue_manager_args(queue_manager_args_t* args, Queue* q, int sd, client_node_t **head, client_node_t **tail, Monitor_t* client_linkedList){
    args->task_queue = q;
    args->sd = sd;
    args->head = head;
    args->tail = tail;
    args->client_linkedList = client_linkedList; 
    args->active_worker = 0;

    pthread_mutex_init(&args->pool_lock, NULL);
    pthread_cond_init(&args->cond_worker, NULL); 
}


typedef struct {
    int sd;
    client_node_t **head;
    client_node_t **tail;
    Monitor_t* client_linkedList; 

    int active_worker; 
    pthread_cond_t cond_worker; 
    pthread_mutex_t pool_lock; 
} connection_manager_args_t;

void setup_connection_manager_args(connection_manager_args_t* args, int sd, client_node_t **head, client_node_t **tail, Monitor_t* client_linkedList){
    args->sd = sd;
    args->head = head;
    args->tail = tail;
    args->client_linkedList = client_linkedList; 
    args->active_worker = 0;

    pthread_mutex_init(&args->pool_lock, NULL);
    pthread_cond_init(&args->cond_worker, NULL); 
}



// muted chat buffer, local to each user
// initialise using new
typedef struct {
    char messages[MAX_NAMES][MAX_NAME_LEN]; 
} muted_namelist_t;

// Monitor subroutines
// reader check-in and check-out routine
void reader_checkin(Monitor_t* client_linkedList){
    pthread_mutex_lock(&client_linkedList->lock); 
    while((client_linkedList->AW + client_linkedList->WW) > 0){
        client_linkedList->WR++; 
        pthread_cond_wait(&client_linkedList->cond_read, &client_linkedList->lock); 
        client_linkedList->WR--; 
    }
    client_linkedList->AR++; 
    pthread_mutex_unlock(&client_linkedList->lock); 
}

void reader_checkout(Monitor_t* client_linkedList){
    pthread_mutex_lock(&client_linkedList->lock); 
    client_linkedList->AR--;  
    if(client_linkedList->AR == 0 && client_linkedList->WW > 0){
        pthread_cond_signal(&client_linkedList->cond_write); // wake up ONE writer
    }
    pthread_mutex_unlock(&client_linkedList->lock); 
}

// writer check-in and check-out routine
void writer_checkin(Monitor_t* client_linkedList){
    pthread_mutex_lock(&client_linkedList->lock); 
    while((client_linkedList->AW + client_linkedList->AR)>0){
        client_linkedList->WW ++;
        pthread_cond_wait(&client_linkedList->cond_write, &client_linkedList->lock); 
    }
    client_linkedList->AW++;
    pthread_mutex_unlock(&client_linkedList->lock); 
}

void writer_checkout(Monitor_t* client_linkedList){
    pthread_mutex_lock(&client_linkedList->lock); 
    client_linkedList->AW--; 
    if (client_linkedList->WW > 0){ // Give priority to writers
        pthread_cond_signal(&client_linkedList->cond_write);// Wake up ONE writer
    } else if (client_linkedList->WR > 0) { // Otherwise, wake reader
        pthread_cond_broadcast(&client_linkedList->cond_read); // Wake ALL readers 
        }
    pthread_mutex_unlock(&client_linkedList->lock);
}

void worker_thread_checkin(queue_manager_args_t* qm_args){
    pthread_mutex_lock(&qm_args->pool_lock);
    while(qm_args->active_worker >= MAX_THREADS){
        pthread_cond_wait(&qm_args->cond_worker, &qm_args->pool_lock); 
    }
    qm_args->active_worker++; 
    pthread_mutex_unlock(&qm_args->pool_lock); 
}

void worker_thread_checkout(queue_manager_args_t* qm_args){
    pthread_mutex_lock(&qm_args->pool_lock); 
    qm_args->active_worker--; 
    pthread_cond_signal(&qm_args->cond_worker); 
    pthread_mutex_unlock(&qm_args->pool_lock); 
}

