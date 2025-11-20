
#include <stdio.h>
#include <stdlib.h>
#include "udp.h"


typedef struct {
    client_node_t* next;
    struct sockaddr_in client_address;
    char client_name[NAME_SIZE];
} client_node_t

void add_client_to_list(client_node_t * tail, struct sockaddr_in *addr, char name[NAME_SIZE]){
    client_node_t * new_node;
    new_node->client_name = name;
    new_node->client_address = addr;
    tail->next = new_node;
}


void *listen(void *queue){
    //takes in the queue and adds the command into it in the form:
    //[]
}

void *queue_thread(){

}


int main(int argc, char *argv[])
{

    // This function opens a UDP socket,
    // binding it to all IP interfaces of this machine,
    // and port number SERVER_PORT
    // (See details of the function in udp.h)
    int sd = udp_socket_open(SERVER_PORT);

    assert(sd > -1);

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