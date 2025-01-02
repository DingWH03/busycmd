#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "shell.h"
#include "builtintools.h"
#include "batch.h"

#define MAX_LINE_LENGTH 1024
#define MAX_LABEL_LENGTH 256
#define MAX_LABELS 100
#define MAX_ARGS 100
#define MAX_VARIABLES 100

typedef struct {
    char label[MAX_LABEL_LENGTH];
    long file_position;
} Label;

typedef struct {
    char *args[MAX_ARGS];
    int argc;
} Command;

static Label labels[MAX_LABELS];
static int label_count = 0;

// 环境变量存储
static char *variables[MAX_VARIABLES];
static int variable_count = 0;

// 当前批处理文件的参数
static char *batch_args[MAX_ARGS];
static int batch_argc = 0;

#define MAX_RESULT_LENGTH 100

// 判断字符串是否是一个有效的数字
int is_number(const char *str) {
    if (*str == '-' || *str == '+') str++; // 允许负数和正数
    if (*str == '\0') return 0;  // 空字符串不是数字

    while (*str) {
        if (!isdigit((unsigned char)*str)) {
            return 0;  // 如果有非数字字符，返回0
        }
        str++;
    }
    return 1;  // 全部字符都是数字
}

char* process_value(const char *input) {
    static char result[MAX_RESULT_LENGTH];  // 用于存储结果
    char operator;
    double num1, num2;

    // 如果输入是单个数字，直接返回该数字
    if (is_number(input)) {
        snprintf(result, sizeof(result), "%s", input);  // 直接返回输入的数字
        return result;
    }

    // 解析输入中的两个数字和运算符
    if (sscanf(input, "%lf%c%lf", &num1, &operator, &num2) != 3) {
        snprintf(result, sizeof(result), "%s", input);  // 如果无法解析为数字和运算符，返回原始输入
        return result;
    }

    // 计算结果
    double res = 0.0;
    switch (operator) {
        case '+':
            res = num1 + num2;
            break;
        case '-':
            res = num1 - num2;
            break;
        case '*':
            res = num1 * num2;
            break;
        case '/':
            if (num2 == 0) {
                snprintf(result, sizeof(result), "Error: Division by zero");
                return result;
            }
            res = num1 / num2;
            break;
        default:
            snprintf(result, sizeof(result), "Error: Invalid operator");
            return result;
    }

    // 将计算结果转换为字符串并返回
    snprintf(result, sizeof(result), "%.2f", res);
    return result;
}

// 替换字符串中的 %%VAR%% 为对应的环境变量值
static void replace_variables(char *dest, const char *src) {
    const char *p = src;
    char var_name[MAX_LABEL_LENGTH];
    char *d = dest;

    while (*p) {
        if (*p == '%' && *(p + 1) != '\0') {
            p++; // 跳过第一个 %
            const char *start = p;
            // 查找下一个 %
            while (*p && *p != '%') {
                p++;
            }
            if (*p == '%') {
                size_t len = p - start;
                if (len < MAX_LABEL_LENGTH) {
                    strncpy(var_name, start, len);
                    var_name[len] = '\0';
                    const char *var_value = get_env_value(var_name);
                    // printf("%s: %s\n", var_name, var_value);
                    if (var_value) {
                        strcpy(d, var_value);
                        d += strlen(var_value);
                    }
                }
                p++; // 跳过结束的 %
            } else {
                // 如果没有找到结束的 %, 直接复制
                *d++ = '%';
                p = start;
            }
        } else {
            *d++ = *p++;
        }
    }
    *d = '\0';
}


// 去除字符串前面的空格
void trim_left_spaces(char *str) {
    // 如果字符串为空或为 NULL，直接返回
    if (str == NULL) return;
    
    // 移动指针，跳过前面的空格字符
    while (*str != '\0' && isspace((unsigned char)*str)) {
        str++;
    }
    
    // 将新的字符串赋值给原始字符串
    printf("Trimmed string: '%s'\n", str);
}


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
    printf("Condition: %s\n", condition);
    
    char var[MAX_LABEL_LENGTH], operator_str[3];
    double value1, value2, env_value;

    // 处理 %VAR%==value 格式的条件
    if (sscanf(condition, "%%%[^%]%% %2s %lf", var, operator_str, &value2) == 3) {
        const char *env_value_str = get_env_value(var); // 使用 get_env_value 替代 getenv
        if (env_value_str == NULL) {
            fprintf(stderr, "Warning: Environment variable '%s' is not set.\n", var);
            return false;
        }

        // 将环境变量的值转换为 double
        env_value = strtod(env_value_str, NULL);

        if (strcmp(operator_str, "==") == 0) {
            return env_value == value2;
        } else if (strcmp(operator_str, "!=") == 0) {
            return env_value != value2;
        }
        // TODO: 处理更多操作符
    } 
    // 处理简单的数字比较（如 1==5）
    else if (sscanf(condition, "%lf %2s %lf", &value1, operator_str, &value2) == 3) {
        // 判断操作符
        if (strcmp(operator_str, "==") == 0) {
            return value1 == value2;
        } else if (strcmp(operator_str, "!=") == 0) {
            return value1 != value2;
        }
        // TODO: 处理更多操作符
    } else {
        fprintf(stderr, "Error: Failed to parse IF condition '%s'\n", condition);
    }
    return false;
}

static bool execute_for_loop(FILE *file, char *line) {
    // Simple FOR /L syntax: FOR /L var start end step
    char var[MAX_LABEL_LENGTH];
    int start, end, step;
    
    // Parse the FOR /L command
    if (sscanf(line, "FOR %s IN (%d %d %d) DO (", var, &start, &end, &step) != 4) {
        fprintf(stderr, "Error: Invalid FOR command. Expected syntax: FOR /L var start end step\n");
        return false;
    }

    // Record the current position as the start of the loop body
    long loop_start_pos = ftell(file);

    // Find the position of )
    long loop_end_pos = -1;
    char loop_line[MAX_LINE_LENGTH];
    while (fgets(loop_line, sizeof(loop_line), file)) {
        // Remove newline characters
        loop_line[strcspn(loop_line, "\r\n")] = 0;
        
        // Check for )
        if (strncmp(loop_line, ")", 1) == 0) {
            loop_end_pos = ftell(file);
            printf("Found ) at position %ld\n", loop_end_pos);
            break;
        }
    }

    if (loop_end_pos == -1) {
        fprintf(stderr, "Error: ) not found for FOR loop starting at position %ld\n", loop_start_pos);
        return false;
    }

    // Determine loop condition based on step
    if ((step > 0 && start > end) || (step < 0 && start < end)) {
        // No iterations needed
        fseek(file, loop_end_pos, SEEK_SET);
        return true;
    }

    // Execute the loop
    for (int i = start; (step > 0) ? (i <= end) : (i >= end); i += step) {
        char value_str[32];
        snprintf(value_str, sizeof(value_str), "%d", i);
        set_environment_variable(var, process_value(value_str)); // Use your environment setter

        // Seek to the start of the loop body
        fseek(file, loop_start_pos, SEEK_SET);

        // Execute each line in the loop body
        while (ftell(file) < loop_end_pos && fgets(loop_line, sizeof(loop_line), file)) {
            // Remove newline characters
            loop_line[strcspn(loop_line, "\r\n")] = 0;
            
            // Skip labels, empty lines, and comments
            if (loop_line[0] == ':' || loop_line[0] == '\0' || loop_line[0] == '#') {
                continue;
            }
            
            // Execute the command
            printf("Executing command inside FOR loop: %s\n", loop_line);
            if (!execute_single_command(loop_line, file)) {
                fprintf(stderr, "Error executing command inside FOR loop: %s\n", loop_line);
                return false;
            }
        }
    }

    // After loop, seek to loop_end_pos to continue execution
    fseek(file, loop_end_pos, SEEK_SET);
    return true;
}

// Corrected execute_while_loop function
static bool execute_while_loop(FILE *file, char *line) {
    // Simple WHILE syntax: WHILE condition
    char condition[MAX_LINE_LENGTH];
    
    // Parse the WHILE command
    if (sscanf(line, "WHILE %[^\n]", condition) != 1) {
        fprintf(stderr, "Error: Invalid WHILE command. Expected syntax: WHILE condition\n");
        return false;
    }

    // Record the current position as the start of the loop body
    long loop_start_pos = ftell(file);

    // Find the position of ENDWHILE
    long loop_end_pos = -1;
    char loop_line[MAX_LINE_LENGTH];
    while (fgets(loop_line, sizeof(loop_line), file)) {
        // Remove newline characters
        loop_line[strcspn(loop_line, "\r\n")] = 0;
        
        // Check for ENDWHILE
        if (strncmp(loop_line, "ENDWHILE", 8) == 0) {
            loop_end_pos = ftell(file);
            break;
        }
    }

    if (loop_end_pos == -1) {
        fprintf(stderr, "Error: ENDWHILE not found for WHILE loop starting at position %ld\n", loop_start_pos);
        return false;
    }

    // Execute the loop
    while (1) {
        // Process variable replacements in the condition
        char processed_condition[MAX_LINE_LENGTH];
        replace_variables(processed_condition, condition);
        
        // Evaluate the condition
        if (!parse_if_condition(processed_condition)) {
            break; // Exit loop if condition is false
        }

        // Seek to the start of the loop body
        fseek(file, loop_start_pos, SEEK_SET);

        // Execute each line in the loop body
        bool any_command_failed = false;
        while (ftell(file) < loop_end_pos && fgets(loop_line, sizeof(loop_line), file)) {
            // Remove newline characters
            loop_line[strcspn(loop_line, "\r\n")] = 0;
            
            // Skip labels, empty lines, and comments
            if (loop_line[0] == ':' || loop_line[0] == '\0' || loop_line[0] == '#') {
                continue;
            }
            
            // Execute the command
            if (!execute_single_command(loop_line, file)) {
                fprintf(stderr, "Error executing command inside WHILE loop: %s\n", loop_line);
                any_command_failed = true;
                break;
            }
        }

        if (any_command_failed) {
            return false;
        }
    }

    // After loop, seek to loop_end_pos to continue execution
    fseek(file, loop_end_pos, SEEK_SET);
    return true;
}


// 执行批处理文件中的单行命令
bool execute_single_command(const char *line, FILE *current_file_pointer) {
    char processed_line[MAX_LINE_LENGTH];
    replace_variables(processed_line, line); // 进行变量替换
    // trim_left_spaces(processed_line); // 去除前导空格

    if (strncmp(processed_line, "GOTO", 4) == 0) {
        char label[MAX_LABEL_LENGTH];
        sscanf(processed_line + 4, "%s", label);
        return goto_label(current_file_pointer, label);
    } else if (strncmp(processed_line, "SET", 3) == 0) {
        char var[MAX_LABEL_LENGTH], value[MAX_LABEL_LENGTH];
        if (sscanf(processed_line + 4, "%[^=]=%s", var, value) == 2) {
            set_environment_variable(var, process_value(value)); 
            // printf("Set %s=%s\n", var, value);
        } else {
            fprintf(stderr, "Error: Invalid SET command\n");
        }
    } else if (strncmp(processed_line, "IF", 2) == 0) {
        char condition[MAX_LINE_LENGTH], action[MAX_LINE_LENGTH];
        if (sscanf(processed_line + 3, "%[^ ] %[^\n]", condition, action) == 2) {
            if (parse_if_condition(condition)) {
                printf("Condition is true: %s\n", action);
                // 修改这里，调用 execute_single_command 而不是 execute_command
                execute_single_command(action, current_file_pointer);
            }
        } else {
            fprintf(stderr, "Error: Invalid IF command\n");
        }
    } else if (strncmp(processed_line, "ECHO", 4) == 0) {
        const char *args[] = { "echo", processed_line + 5, NULL };
        builtin_echo((char **)args);
    } else if (strncmp(processed_line, "SHIFT", 5) == 0) {
        // 实现 SHIFT 命令
        if (batch_argc > 0) {
            for (int i = 1; i < batch_argc; ++i) {
                batch_args[i - 1] = batch_args[i];
            }
            batch_args[batch_argc - 1] = NULL;
            batch_argc--;
        }
    } else if (strncmp(processed_line, "FOR", 3) == 0) {
        // 实现 FOR 循环
        return execute_for_loop(current_file_pointer, processed_line);
    } else if (strncmp(processed_line, "WHILE", 5) == 0) {
        // 实现 WHILE 循环
        return execute_while_loop(current_file_pointer, processed_line);
    } else {
        // 执行外部命令
        return execute_command(processed_line);
    }
    return true;
}


// 解析 .batch 文件
bool execute_batch_file(const char *filename) {
    load_system_env(); // 加载系统环境变量
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening batch file");
        return false;
    }

    char line[MAX_LINE_LENGTH];
    label_count = 0;

    // 第一次遍历，收集标号
    while (fgets(line, sizeof(line), file)) {
        long pos = ftell(file);
        line[strcspn(line, "\r\n")] = 0; // 去掉换行符
        if (line[0] == ':') {           // 标号
            if (label_count >= MAX_LABELS) {
                fprintf(stderr, "Error: Too many labels\n");
                fclose(file);
                return false;
            }
            strncpy(labels[label_count].label, line + 1, MAX_LABEL_LENGTH - 1);
            labels[label_count].label[MAX_LABEL_LENGTH - 1] = '\0';
            labels[label_count].file_position = pos;
            label_count++;
        }
    }

    rewind(file);

    // 第二次遍历，执行命令
    while (fgets(line, sizeof(line), file)) {
        long current_pos = ftell(file);
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
        } else if (strncmp(line, "FOR", 3) == 0) {
            if (!execute_for_loop(file, line)) {
                fclose(file);
                return false;
            }
        } else if (strncmp(line, "WHILE", 5) == 0) {
            if (!execute_while_loop(file, line)) {
                fclose(file);
                return false;
            }
        } else {
            if (!execute_single_command(line, file)) {
                fprintf(stderr, "Error executing command: %s\n", line);
                fclose(file);
                return false;
            }
        }
    }

    fclose(file);
    return true;
}
