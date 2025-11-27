#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>

#define QUEUE_MAX 256
#define QUEUE_MSG_SIZE 512
#define MAX_COMMAND_LEN 3


typedef struct {
    char msg[QUEUE_MSG_SIZE];
    struct sockaddr_in from_addr;
} queue_node_t;

typedef struct {
    queue_node_t data[QUEUE_MAX];
    int head;
    int tail;
    int size;
    pthread_mutex_t lock;
    pthread_cond_t nonempty;
    pthread_cond_t nonfull;
} Queue;

void queue_init(Queue *q){
    q->head = 0;
    q->tail = 0;
    q->size = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->nonempty, NULL);
    pthread_cond_init(&q->nonfull, NULL);

}

void get_and_tokenise(queue_node_t *node, char * args[]){
    
    char *token = strtok(node->msg, " "); 
    int argsc = 0; 
    while (token != NULL && argsc < MAX_COMMAND_LEN) // max len command = sayto, to, msg = len 3
    {
        args[(argsc)++] = token;
        token = strtok(NULL, " ");
    }

    args[argsc] = NULL; //args must be null terminated

}

//add to the thread and give nonempty signal under mutex
void q_append(Queue* q, char* msg, struct sockaddr_in from_addr){
    
    pthread_mutex_lock(&q->lock);
    while ((q->tail + 1) % QUEUE_MAX == q->head) { // queue full
        pthread_cond_wait(&q->nonfull, &q->lock);
    }   
    
    queue_node_t *node = &q->data[q->tail];

    strncpy(node->msg, msg, QUEUE_MSG_SIZE);
    node->from_addr = from_addr;

    q->tail = (q->tail + 1) % QUEUE_MAX;
    q->size++;

    pthread_cond_signal(&q->nonempty);
    pthread_mutex_unlock(&q->lock);
}

//put the current thread to sleep based on if the queue is empty then store head tokenised elements in out arr
void q_pop(Queue * q, char * out[], struct sockaddr_in * sender){
    pthread_mutex_lock(&q->lock);

    while (q->head == q->tail){
        pthread_cond_wait(&q->nonempty, &q->lock);
    }

    int idx = q->head;
    queue_node_t *node = &q->data[idx];
    get_and_tokenise(node, out);
    *sender = node->from_addr;

    q->head = (q->head + 1) % QUEUE_MAX;
    q->size--;
    
    pthread_cond_signal(&q->nonfull);
    pthread_mutex_unlock(&q->lock);
}
