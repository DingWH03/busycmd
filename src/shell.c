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
#include <string.h>

EnvVar shell_env[MAX_ENV_VARS];
int env_count = 0;
char current_directory[MAX_PATH_LEN];

extern char **environ;

void load_system_env() {
    for (char **env = environ; *env != NULL; ++env) {
        char *entry = strdup(*env);
        char *delimiter = strchr(entry, '=');

        if (delimiter) {
            *delimiter = '\0';
            const char *name = entry;
            const char *value = delimiter + 1;

            if (env_count < MAX_ENV_VARS) {
                shell_env[env_count].name = strdup(name);
                shell_env[env_count].value = strdup(value);
                env_count++;
            } else {
                fprintf(stderr, "Environment variable limit reached. Skipping: %s\n", name);
            }
        }

        free(entry);
    }
}

char **build_env_array() {
    char **env_array = malloc((env_count + 1) * sizeof(char *));
    for (int i = 0; i < env_count; ++i) {
        size_t len = strlen(shell_env[i].name) + strlen(shell_env[i].value) + 2;
        env_array[i] = malloc(len);
        snprintf(env_array[i], len, "%s=%s", shell_env[i].name, shell_env[i].value);
    }
    env_array[env_count] = NULL;
    return env_array;
}

// 查找环境变量的值
const char* get_env_value(const char* name) {
    for (int i = 0; i < env_count; ++i) {
        if (strcmp(shell_env[i].name, name) == 0) {
            return shell_env[i].value;  // 找到匹配的环境变量，返回值
        }
    }
    return "";  // 如果没有找到，返回空字符串
}

// 设置环境变量
void set_environment_variable(const char *name, const char *value) {
    // 查找该环境变量是否已经存在
    for (int i = 0; i < env_count; ++i) {
        if (strcmp(shell_env[i].name, name) == 0) {
            // 环境变量已存在，更新其值
            free(shell_env[i].value);  // 释放原有的值
            shell_env[i].value = strdup(value);  // 更新为新值
            return;
        }
    }

    // 如果该环境变量不存在，添加新的环境变量
    if (env_count < MAX_ENV_VARS) {
        shell_env[env_count].name = strdup(name);
        shell_env[env_count].value = strdup(value);
        env_count++;
    } else {
        fprintf(stderr, "Environment variable limit reached. Cannot set: %s\n", name);
    }
}

// 删除环境变量
void unset_environment_variable(const char *name) {
    for (int i = 0; i < env_count; ++i) {
        if (strcmp(shell_env[i].name, name) == 0) {
            // 找到并删除环境变量
            free(shell_env[i].name);    // 释放名称内存
            free(shell_env[i].value);   // 释放值内存

            // 将剩余的环境变量移动到当前位置
            for (int j = i; j < env_count - 1; ++j) {
                shell_env[j] = shell_env[j + 1];
            }

            env_count--;  // 更新环境变量计数
            return;
        }
    }
    fprintf(stderr, "Environment variable not found: %s\n", name);
}


void free_env_array(char **env_array) {
    if (env_array == NULL) {
        return;
    }
    
    for (int i = 0; i < env_count; ++i) {
        free(env_array[i]);
    }
    
    free(env_array);
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

    // 检查是否执行内置命令
    if (is_builtin_command(args[0])) {
        execute_builtin(args);
    } else if(SUPPORT_EXTERNAL_COMMANDS) { // 判断是否执行外部命令
        // 执行外部命令
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
        } else if (pid == 0) {
            char **env = build_env_array();
            if (execvpe(args[0], args, env) == -1) {
                perror("Error executing command");
                free_env_array(env);
                exit(EXIT_FAILURE);
            }
            free_env_array(env);
        } else {
            int status;
            waitpid(pid, &status, 0);
        }

    }
    else {
        fprintf(stderr, "Command not found: %s\n", args[0]);
    }

    free(command_copy);
    return true;
}

void shell_loop() {
    char input[INPUT_BUFFER_SIZE];
    if (!getcwd(current_directory, MAX_PATH_LEN)){
        perror("getcwd() error");
    }
    load_system_env(); // 加载系统环境变量
    // print_shell_env();

    printf("Welcome to %s\n", SHELL_PROMPT);
    while (1) {
        // 打印提示符
        printf("%s: %s>", SHELL_PROMPT, current_directory);
        fflush(stdout);

        console_input(input);

        // 空输入跳过执行
        if (strlen(input) == 0) {
            continue;
        }

        // 执行命令
        execute_command(input);
    }
}

