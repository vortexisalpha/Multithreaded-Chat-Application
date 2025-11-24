#define MAX_NAME 30
#define MAX_MESSAGE 256
#define MAX_SIZE 100
#define MAX_ACTIVITY 1000
#define MAX_ACTIVITY_LOG 1024

typedef struct{
    char name[MAX_NAME]; // from who
    bool private_msg; // is this a private message (show line: only delivered to Bob or sth equivalent)
    char message[MAX_MESSAGE]; 
    int rear;
    int front; 
    int size; 
} message_Queue; // only concerns to the receiver thread of client side. 


void queue_init(message_Queue *q){
    q->front = -1;
    q->rear = 0;
    q->size = 0;
}

bool isEmpty(message_Queue* q) { return (q->front == q->rear - 1); }

bool isFull(message_Queue* q) { return (q->rear == MAX_SIZE); }


char activity_history[MAX_ACTIVITY][MAX_ACTIVITY_LOG]; 

