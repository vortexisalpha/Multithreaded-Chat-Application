#ifndef COMMAND_H
#define COMMAND_H

#include "udp.h"
#define MAX_MESSAGE 256
#define RESPONSE_BUFFER_SIZE (MAX_MESSAGE + 256)  // Buffer for formatted responses with usernames

typedef enum {
    CONN,
    CONN_SUCCESS,    // Server response: connection successful
    CONN_FAILED,     // Server response: connection failed
    SAY,
    SAYTO,
    MUTE,
    UNMUTE,
    RENAME,
    DISCONN,
    DISCONN_RESPONSE, // Server response: disconnection confirmed
    KICK,
    PING,            // Server to client: ping request
    RETPING,          // Client to server: ping response
    ERROR,           // Server response: error message
    UNKNOWN          // Unknown/invalid command
} command_kind_t;

typedef struct {
    command_kind_t kind;
    char  args[MAX_CMD_SIZE][NAME_SIZE];    
} command_t;

void command_handler(command_t *command, char *args[]){
    if (args == NULL || args[0] == NULL) return;
    if (strcmp(args[0], "conn") == 0) {
        command->kind = CONN;
    } else if (strcmp(args[0], "connsuccess") == 0) {
        command->kind = CONN_SUCCESS;
    } else if (strcmp(args[0], "connfailed") == 0) {
        command->kind = CONN_FAILED;
    } else if (strcmp(args[0], "say") == 0) {
        command->kind = SAY;
    } else if (strcmp(args[0], "sayto") == 0) {
        command->kind = SAYTO;
    } else if (strcmp(args[0], "mute") == 0) {
        command->kind = MUTE;
    } else if (strcmp(args[0], "unmute") == 0) {
        command->kind = UNMUTE;
    } else if (strcmp(args[0], "rename") == 0) {
        command->kind = RENAME;
    } else if (strcmp(args[0], "disconn") == 0) {
        command->kind = DISCONN;
    } else if (strcmp(args[0], "disconnresponse") == 0) {
        command->kind = DISCONN_RESPONSE;
    } else if (strcmp(args[0], "kick") == 0) {
        command->kind = KICK;
    } else if (strcmp(args[0], "ping") == 0) {
        command->kind = PING;
    } else if (strcmp(args[0], "retping") == 0) {
        command->kind = RETPING;
    } else if (strcmp(args[0], "error") == 0) {
        command->kind = ERROR;
    } else {
        // unknown command
        command->kind = UNKNOWN;
    }

    int i = 1;   // skip command name
    int out = 0;

    while (args[i] != NULL && out < MAX_CMD_SIZE) {
        strncpy(command->args[out], args[i], NAME_SIZE - 1);
        command->args[out][NAME_SIZE-1] = '\0';
        out++;
        i++;
    }

    // clear remaining slots
    while (out < MAX_CMD_SIZE) {
        command->args[out][0] = '\0';
        out++;
    }

    /* Some example of the command call
        CLIENT TO SERVER:
        1. conn$ client_name (arg0~1)
        2. say$ msg (arg0~1)
        3. sayto$ recipient_name msg (arg0~2)
        4. disconn$ (arg0)
        5. mute$ client_name (arg0~1)
        6. unmute$ client_name (arg0~1)
        7. rename$ new_name (arg0~1)
        8. kick$ client_name (arg0~1)
        9. unknown command (args dont matter)

        SERVER TO CLIENT:
        1. connsuccess$ client_name (arg0~1) - connection successful
        2. connfailed$ error_message (arg0~1) - connection failed
        3. disconnresponse$ (arg0) - disconnect confirmed
        4. error$ error_message (arg0~1) - invalid command error
        5. say$ sender: message (arg0~N) - broadcast message
        6. sayto$ sender: message (arg0~N) - broadcast message
        7. mute$ name (arg0) - add to mute list
        8. unmute$ name (arg0) - remove from mute list
        8. error$ error_code
    */
}
#endif
