#include "batch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"
#include "builtintools.h"

#define MAX_LINE_LENGTH 1024
#define MAX_LABEL_LENGTH 256

typedef struct {
    char label[MAX_LABEL_LENGTH];
    long file_position;
} Label;

#define MAX_LABELS 100
static Label labels[MAX_LABELS];
static int label_count = 0;

// 跳转到指定标号
static bool goto_label(FILE *file, const char *label_name) {
    for (int i = 0; i < label_count; ++i) {
        if (strcmp(labels[i].label, label_name) == 0) {
            fseek(file, labels[i].file_position, SEEK_SET);
            return true;
        }
    }
    fprintf(stderr, "Error: Label '%s' not found\n", label_name);
    return false;
}

// 解析 IF 条件
static bool parse_if_condition(const char *condition) {
    // 示例：%VAR%==value
    char var[MAX_LABEL_LENGTH], value[MAX_LABEL_LENGTH];
    if (sscanf(condition, "%%%[^=]=%s", var, value) == 2) {
        const char *env_value = getenv(var);
        return env_value && strcmp(env_value, value) == 0;
    }
    return false;
}

// 解析 .batch 文件
bool execute_batch_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening batch file");
        return false;
    }

    char line[MAX_LINE_LENGTH];
    label_count = 0;

    // 第一次遍历，收集标号
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0; // 去掉换行符
        if (line[0] == ':') {           // 标号
            if (label_count >= MAX_LABELS) {
                fprintf(stderr, "Error: Too many labels\n");
                fclose(file);
                return false;
            }
            strncpy(labels[label_count].label, line + 1, MAX_LABEL_LENGTH - 1);
            labels[label_count].label[MAX_LABEL_LENGTH - 1] = '\0';
            labels[label_count].file_position = ftell(file);
            label_count++;
        }
    }

    rewind(file);

    // 第二次遍历，执行命令
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0; // 去掉换行符

        if (line[0] == '\0' || line[0] == '#') continue; // 忽略空行或注释

        if (line[0] == ':') {
            // 标号，无需执行
            continue;
        } else if (strncmp(line, "GOTO", 4) == 0) {
            char label[MAX_LABEL_LENGTH];
            sscanf(line + 4, "%s", label);
            if (!goto_label(file, label)) {
                fclose(file);
                return false;
            }
        } else if (strncmp(line, "SET", 3) == 0) {
            char var[MAX_LABEL_LENGTH], value[MAX_LABEL_LENGTH];
            if (sscanf(line + 3, "%[^=]=%s", var, value) == 2) {
                setenv(var, value, 1);
            } else {
                fprintf(stderr, "Error: Invalid SET command\n");
            }
        } else if (strncmp(line, "IF", 2) == 0) {
            char condition[MAX_LABEL_LENGTH], action[MAX_LINE_LENGTH];
            if (sscanf(line + 2, "%[^ ] %[^\n]", condition, action) == 2) {
                if (parse_if_condition(condition)) {
                    execute_command(action);
                }
            } else {
                fprintf(stderr, "Error: Invalid IF command\n");
            }
        } else if (strncmp(line, "ECHO", 4) == 0) {
            const char *args[] = { "echo", line + 5, NULL };
            builtin_echo(args);
        } else if (strncmp(line, "SHIFT", 5) == 0) {
            fprintf(stderr, "SHIFT command not implemented yet\n");
        } else if (strncmp(line, "FOR", 3) == 0) {
            fprintf(stderr, "FOR command not implemented yet\n");
        } else if (strncmp(line, "WHILE", 5) == 0) {
            fprintf(stderr, "WHILE command not implemented yet\n");
        } else {
            if (!execute_command(line)) {
                fprintf(stderr, "Error executing command: %s\n", line);
                fclose(file);
                return false;
            }
        }
    }

    fclose(file);
    return true;
}
