#include <stdbool.h>
#include <string.h>
#include "builtintools.h"

const char *builtin_commands[NUM_BUILTINS] = {
    "echo"
};

bool is_builtin_command(const char *command) {
    for (int i = 0; i < NUM_BUILTINS; i++) {
        if (strcmp(command, builtin_commands[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool execute_builtin(char **command) {
    if (strcmp(command[0], "echo") == 0) {
        return builtin_echo((const char **)command);
    }
    return false;
}