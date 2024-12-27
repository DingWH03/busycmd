#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "shell.h"
#include "builtintools.h"
#include "utils/path_utils.h"
#include "utils/input.h"
#include "config.h"
#include <stdbool.h>

void shell_loop() {
    char input[INPUT_BUFFER_SIZE];

    printf("Welcome to Custom Shell!\n");
    while (1) {
        // Print prompt
        printf("%s", SHELL_PROMPT);
        fflush(stdout);

        console_input(input);

        // Skip empty input
        if (strlen(input) == 0) {
            continue;
        }

        // Execute the command
        execute_command(input);
    }
}

bool execute_command(const char *command) {
    char *args[INPUT_BUFFER_SIZE / 2 + 1];
    char *token;
    int i = 0;

    // Tokenize the input string
    char *command_copy = strdup(command);
    if (!command_copy) {
        perror("strdup failed");
        return false;
    }

    token = strtok(command_copy, " ");
    while (token != NULL && i < (INPUT_BUFFER_SIZE / 2)) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    if (i == 0) {
        free(command_copy);
        return false;
    }

    // Check if the command is a builtin
    if (is_builtin_command(args[0])) {
        execute_builtin(args);
    } else {
        // Attempt to execute as an external command
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
        } else if (pid == 0) {
            // Child process
            if (execvp(args[0], args) == -1) {
                perror("Error executing command");
                exit(EXIT_FAILURE);
            }
        } else {
            // Parent process, wait for child to complete
            int status;
            waitpid(pid, &status, 0);
        }
    }

    free(command_copy);
    return true;
}
