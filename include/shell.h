#ifndef SHELL_H
#define SHELL_H
#include <stdbool.h>
#include "config.h"

typedef struct {
    char *name;
    char *value;
} EnvVar;

extern EnvVar shell_env[MAX_ENV_VARS];
extern int env_count;
extern char current_directory[MAX_PATH_LEN];

// Function prototypes
void set_environment_variable(const char *name, const char *value);
void unset_environment_variable(const char *name);
// 查找环境变量的值
const char* get_env_value(const char* name);

/**
 * Starts the shell loop, reading and executing commands until exit.
 */
void shell_loop();
bool execute_command(const char *command);

#endif // SHELL_H
