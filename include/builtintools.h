#ifndef BUILTINTOOLS_H
#define BUILTINTOOLS_H

#include <stdbool.h>

#define NUM_BUILTINS 7

extern const char *builtin_commands[NUM_BUILTINS];


// Function prototypes

/**
 * Checks if a given command is a builtin command.
 *
 * @param command The command to check.
 * @return true if the command is a builtin, false otherwise.
 */
bool is_builtin_command(const char *command);

/**
 * Executes a builtin command.
 *
 * @param args The arguments for the command, including the command itself as args[0].
 */

// 检查并执行内置命令
bool execute_builtin(char **command);

// 内置命令原型
bool builtin_echo(const char **args);

void change_directory(const char *path);

void print_shell_env();


void exit_shell();

#endif
