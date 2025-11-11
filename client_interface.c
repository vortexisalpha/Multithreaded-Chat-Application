#include <stdio.h>
#include "udp.h"

#define CLIENT_PORT 10000
#define MAX_MSGS 100
#define MAX_LEN 1024

void clear_screen() {
    printf("\033[2J\033[H"); // Clear screen + move cursor to top-left
}


void tokenise_input(char input[] ,char *args[]){
    char *token = strtok(input, "$");
    args[0] = token;
    printf("%s\n", token);
    token = strtok(NULL, "$");
    printf("%s\n", token);

    if (token != NULL){
        memmove(token, token + 1, strlen(token));
        args[1] = token; // removes the space
    }
}

// client code
int main(int argc, char *argv[])
{

    char messages[MAX_MSGS][MAX_LEN];
    int message_count = 0;

    char input[MAX_LEN];

    while(1){
        clear_screen();
        printf("Chat Messages\n\n");

        for (int i = 0; i < message_count; i++){
            printf("%s\n", messages[i]);
        }

        printf("-----------------------------------\n");


        //sender thread stuff
        printf("> ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0; // remove \n when enter is pressed
        char *args[2];
    

        if (strcmp(input, ":q") == 0) break;

        if (message_count < MAX_MSGS){
            tokenise_input(input, args);//error check input later
            snprintf(messages[message_count], MAX_LEN + 6, "You: %s and %s", args[0], args[1]); // This is how we print to messages.
            message_count++;
        }


        //listner thread stuff
    }
    return 0;
}