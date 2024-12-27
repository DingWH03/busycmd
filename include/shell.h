#ifndef SHELL_H
#define SHELL_H
#include <stdbool.h>

// Function prototypes

/**
 * Starts the shell loop, reading and executing commands until exit.
 */
void shell_loop();
bool execute_command(const char *command);

#endif // SHELL_H
