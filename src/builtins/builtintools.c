#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>  
#include "builtintools.h"
#include "shell.h"

const char *builtin_commands[NUM_BUILTINS] = {
    "echo",
    "exit",
    "env",
    "cd",
    "set",
    "unset",
    "cls"
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
    if (strcmp(command[0], "exit") == 0) {
    exit_shell();
    }
    else if (strcmp(command[0], "echo") == 0) {
        return builtin_echo((const char **)command);
    }
    else if (strcmp(command[0], "cd") == 0) {
        if (command[1] == NULL) {
            change_directory(get_env_value("HOME"));
        } else {
            change_directory(command[1]);
        }
        return true;
    }
    else if (strcmp(command[0], "set") == 0) {
    // 检查命令格式是否正确，即需要有两个参数（变量名和变量值）
    if (command[1] != NULL && command[2] != NULL) {
        set_environment_variable(command[1], command[2]);  // 调用之前实现的设置环境变量函数
        printf("Environment variable '%s' set to '%s'\n", command[1], command[2]);
    } else {
        printf("Usage: set <variable_name> <value>\n");
    }
    }
    else if (strcmp(command[0], "unset") == 0) {
        // 检查命令格式是否正确，即需要有一个参数（变量名）
        if (command[1] != NULL) {
            unset_environment_variable(command[1]);  // 调用之前实现的删除环境变量函数
            printf("Environment variable '%s' has been unset\n", command[1]);
        } else {
            printf("Usage: unset <variable_name>\n");
        }
    }
    else if (strcmp(command[0], "cls") == 0) {
        printf("\033[2J\033[1;1H");
        return true;
    }

    else if (strcmp(command[0], "env") == 0) {
        print_shell_env();
        return true;
    }
    return false;
}

void print_shell_env() {
    printf("Loaded Environment Variables:\n");
    for (int i = 0; i < env_count; ++i) {
        printf("%s=%s\n", shell_env[i].name, shell_env[i].value);
    }
}

void change_directory(const char *path) {
    if (chdir(path) != 0) {
        perror("cd failed");
    } else if (!getcwd(current_directory, MAX_PATH_LEN)) {
        perror("getcwd failed");
    }
}

void exit_shell() {
    printf("Bye Bye!\n");
    exit(0);
}