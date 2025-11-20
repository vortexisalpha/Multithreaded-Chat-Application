#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#define QUEUE_MAX 256
#define QUEUE_MSG_SIZE 512

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

void q_append(Queue* q, char* msg){
    pthread_mutex_lock(&q->lock);
    q->size++;
    strncpy(q->data[q->tail], msg, QUEUE_MSG_SIZE);
    q->tail = (q->tail + 1) % QUEUE_MAX;

    pthread_cond_signal(&q->nonempty);
    pthread_mutex_unlock(&q->lock);
}

void q_pop(Queue * q, char * out){
    pthread_mutex_lock(&q->lock);
    
}

void get_and_tokenise(Queue * q, int idx, char * args){

}