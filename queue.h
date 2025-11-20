#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#define QUEUE_MAX 256
#define QUEUE_MSG_SIZE 512
#define MAX_COMMAND_LEN 4


typedef struct {
    char data[QUEUE_MAX][QUEUE_MSG_SIZE];
    int head;
    int tail;
    int size;
    pthread_mutex_t lock;
    pthread_cond_t nonempty;
} Queue;

void queue_init(Queue *q){
    q->head = 0;
    q->tail = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->nonempty, NULL);
}

void get_and_tokenise(Queue * q, int idx, char * args[]){

    char *token = strtok(q->data[idx], " "); 
    int argsc = 0; 
    while (token != NULL && argsc < MAX_COMMAND_LEN) // max len command = sayto, from, to, msg = len 4
    {
        args[(argsc)++] = token;
        token = strtok(NULL, " ");
    }

    args[argsc] = NULL; //args must be null terminated

}

//add to the thread and give nonempty signal under mutex
void q_append(Queue* q, char* msg){
    pthread_mutex_lock(&q->lock);
    q->size++;
    strncpy(q->data[q->tail], msg, QUEUE_MSG_SIZE);
    q->tail = (q->tail + 1) % QUEUE_MAX;

    pthread_cond_signal(&q->nonempty);
    pthread_mutex_unlock(&q->lock);
}

//put the current thread to sleep based on if the queue is empty then store head tokenised elements in out arr
void q_pop(Queue * q, char * out[]){
    pthread_mutex_lock(&q->lock);

    while (q->head == q->tail){
        pthread_cond_wait(&q->nonempty, &q->lock);
    }

    q->head = (q->head + 1) % QUEUE_MAX;
    q->size--;
    char * head_data;
    
    get_and_tokenise(q, q->head, out);
    
    pthread_mutex_unlock(&q->lock);
}

