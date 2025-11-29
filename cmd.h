#ifndef COMMAND_HEADER
#define COMMAND_HEADER

#include "udp.h"
#define MAX_MESSAGE 256

typedef enum {
    CONN,
    SAY,
    SAYTO,
    MUTE,
    UNMUTE,
    RENAME,
    DISCONN,
    KICK
} command_kind_t;

typedef struct {
    command_kind_t kind;
    char  args[MAX_CMD_SIZE][NAME_SIZE];    
} command_t;

void command_handler(command_t *command, char *args[]){
    if (strcmp(args[0], "conn") == 0) {
        command->kind = CONN;
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
    } else if (strcmp(args[0], "kick") == 0) {
        command->kind = KICK;
    } else {
        // unknown command -> undefined behaviour atm
        return;
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
        1. conn$ client_name (arg0~1)
        2. say$ msg (arg0~1)
        3. sayto$ recipient_name msg (arg0~2)
        4. disconn$ (arg0)
        5. mute$ client_name (arg0~1)
        6. unmute$ client_name (arg0~1)
        7. rename$ new_name (arg0~1)
        8. kick$ client_name (arg0~1)
    */
}
#endif
