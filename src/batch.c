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
char *batch_args[MAX_ARGS];
int batch_argc = 0;

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
// 修改后的 replace_variables 函数，增加了 batch_args 和 batch_argc 参数
static void replace_variables(char *dest, const char *src) {
    const char *p = src;
    char var_name[MAX_LABEL_LENGTH];
    char *d = dest;

    // 移动指针，跳过前面的空格字符
    while (*p != '\0' && isspace((unsigned char)*p)) {
        p++;
    }

    while (*p) {
        if (*p == '%') {
            const char *start = p + 1; // 跳过第一个 '%'

            // 判断下一个字符是否也是 '%', 以支持 %%VAR%% 形式
            if (*start == '%') {
                // 处理 %%VAR%% 形式
                start++; // 跳过第二个 '%'
                const char *var_start = start;

                // 查找下一个 '%'
                while (*start && *start != '%') {
                    start++;
                }

                if (*start == '%') {
                    size_t len = start - var_start;
                    if (len < MAX_LABEL_LENGTH) {
                        strncpy(var_name, var_start, len);
                        var_name[len] = '\0';

                        const char *var_value = get_env_value(var_name);
                        // 调试输出
                        // printf("Environment Variable Found: %s = %s\n", var_name, var_value ? var_value : "NULL");

                        if (var_value) {
                            strcpy(d, var_value);
                            d += strlen(var_value);
                        }
                    }
                    p = start + 1; // 跳过结束的 '%'
                } else {
                    // 如果没有找到结束的 '%', 复制所有字符
                    strcpy(d, "%%");
                    d += 2;
                    p = var_start;
                }
            } else if (isdigit((unsigned char)*start)) {
                // 处理 %1, %2 等批处理参数
                const char *arg_start = start;
                while (isdigit((unsigned char)*start)) {
                    start++;
                }
                size_t len = start - arg_start;
                if (len > 0 && len < MAX_LABEL_LENGTH) {
                    strncpy(var_name, arg_start, len);
                    var_name[len] = '\0';
                    int arg_index = atoi(var_name) - 1; // 批处理参数从1开始

                    // 调试输出
                    // printf("Batch Argument Found: %s (Index: %d)\n", var_name, arg_index);

                    if (arg_index >= 0 && arg_index < batch_argc) {
                        const char *arg_value = batch_args[arg_index];
                        if (arg_value) {
                            strcpy(d, arg_value);
                            d += strlen(arg_value);
                        }
                    }
                }

                // 如果后面有 '%', 跳过它
                if (*start == '%') {
                    p = start + 1;
                } else {
                    p = start;
                }
            } else {
                // 单独的 '%'，直接复制
                *d++ = *p++;
            }
        } else {
            *d++ = *p++;
        }
    }
    *d = '\0';
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

// 比较函数实现
bool compare(const char *str1, const char *str2, const char *operator) {
    if (str1 == NULL || str2 == NULL || operator == NULL) {
        // 参数无效
        return false;
    }

    if (is_number(str2)) {
        // 如果 str2 是数字，进行数值比较
        char *endptr1, *endptr2;
        double num1 = strtod(str1, &endptr1);
        double num2 = strtod(str2, &endptr2);

        // 检查 str1 是否也是有效的数字
        if (*endptr1 != '\0') {
            // str1 不是有效的数字
            return false;
        }

        // 根据 operator 进行比较
        if (strcmp(operator, "==") == 0) {
            return num1 == num2;
        } else if (strcmp(operator, "!=") == 0) {
            return num1 != num2;
        } else if (strcmp(operator, ">") == 0) {
            return num1 > num2;
        } else if (strcmp(operator, "<") == 0) {
            return num1 < num2;
        } else if (strcmp(operator, ">=") == 0) {
            return num1 >= num2;
        } else if (strcmp(operator, "<=") == 0) {
            return num1 <= num2;
        } else {
            // 不支持的运算符
            fprintf(stderr, "Unsupported operator: %s\n", operator);
            return false;
        }
    } else {
        // 如果 str2 不是数字，进行字符串比较（仅支持 "==" 和 "!="）
        if (strcmp(operator, "==") == 0) {
            return strcmp(str1, str2) == 0;
        } else if (strcmp(operator, "!=") == 0) {
            return strcmp(str1, str2) != 0;
        } else {
            // 不支持的运算符
            fprintf(stderr, "Unsupported operator for string comparison: %s\n", operator);
            return false;
        }
    }
}

static bool parse_if_condition(const char *condition) {
    // printf("Condition: %s\n", condition);
    
    char var[MAX_LABEL_LENGTH], operator_str[3];
    char value1[MAX_LABEL_LENGTH], value2[MAX_LABEL_LENGTH];
    const char *op_position = NULL;
    
    // 支持的操作符列表
    const char* operators[] = {"==", "!=", ">=", "<=", ">", "<"};
    int num_operators = sizeof(operators) / sizeof(operators[0]);
    
    // 查找操作符
    for(int i = 0; i < num_operators; i++) {
        op_position = strstr(condition, operators[i]);
        if(op_position != NULL) {
            strcpy(operator_str, operators[i]);
            break;
        }
    }
    
    if(op_position == NULL) {
        fprintf(stderr, "Error: No valid operator found in condition '%s'\n", condition);
        return false;
    }
    
    // 分割左操作数和右操作数
    size_t lhs_length = op_position - condition;
    strncpy(value1, condition, lhs_length);
    value1[lhs_length] = '\0';
    
    strcpy(value2, op_position + strlen(operator_str));
    
    // 去除可能的引号和空格
    // 这里可以根据需要添加更多的处理
    // 例如去除引号:
    if(value1[0] == '\"') {
        memmove(value1, value1+1, strlen(value1));
        value1[strlen(value1)-1] = '\0';
    }
    if(value2[0] == '\"') {
        memmove(value2, value2+1, strlen(value2));
        value2[strlen(value2)-1] = '\0';
    }
    
    // 处理 %VAR% 格式
    if(value1[0] == '%' && value1[strlen(value1)-1] == '%') {
        value1[strlen(value1)-1] = '\0';
        strcpy(var, value1 + 1);
        const char *env_value_str = get_env_value(var);
        if (env_value_str == NULL) {
            fprintf(stderr, "Warning: Environment variable '%s' is not set.\n", var);
            return false;
        }
        return compare(env_value_str, process_value(value2), operator_str);
    } else {
        return compare(value1, process_value(value2), operator_str);
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

    // 记录循环体的开始位置
    long loop_start_pos = ftell(file);

    // Find the position of )
    long loop_end_pos = -1;
    char loop_line[MAX_LINE_LENGTH];
    long current_pos = ftell(file); // 初始化current_pos

    while (fgets(loop_line, sizeof(loop_line), file)) {
        // 移除换行符
        loop_line[strcspn(loop_line, "\r\n")] = 0;
        
        // Check for )
        if (strncmp(loop_line, ")", 1) == 0) {
            loop_end_pos = current_pos; // Set to position before ')'
            // printf("Found ) at position %ld\n", loop_end_pos);
            break;
        }
        
        current_pos = ftell(file); // 更新current_pos
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

    // 执行循环
    for (int i = start; (step > 0) ? (i <= end) : (i >= end); i += step) {
        char value_str[32];
        snprintf(value_str, sizeof(value_str), "%d", i);
        set_environment_variable(var, process_value(value_str)); // 放在环境变量中

        // 寻找循环体的开始
        fseek(file, loop_start_pos, SEEK_SET);

        // 执行循环体内每一行指令
        while (ftell(file) < loop_end_pos && fgets(loop_line, sizeof(loop_line), file)) {
            // Remove newline characters
            loop_line[strcspn(loop_line, "\r\n")] = 0;
            
            // Skip labels, empty lines, and comments
            if (loop_line[0] == ':' || loop_line[0] == '\0' || loop_line[0] == '#') {
                continue;
            }
            
            // 执行指令
            // printf("Executing command inside FOR loop: %s\n", loop_line);
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


// 执行批处理文件中的单行命令
bool execute_single_command(const char *line, FILE *current_file_pointer) {
    char processed_line[MAX_LINE_LENGTH];
    replace_variables(processed_line, line); // 进行变量替换
    // 移动指针，跳过前面的空格字符
    // printf("Executing command: %s\n", processed_line);

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
                // printf("Condition is true: %s\n", action);
                // 修改这里，调用 execute_single_command 而不是 execute_command
                execute_single_command(action, current_file_pointer);
            }
        } else {
            fprintf(stderr, "Error: Invalid IF command\n");
        }
    } else if (strncmp(processed_line, "ECHO", 4) == 0) {
        const char *args[] = { "echo", processed_line + 5, NULL };
        builtin_echo((const char **)args);
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
    } else if(is_builtin_command(processed_line)) {
        // 实现其他内置命令
        char *args[MAX_ARGS];
        int argc = 0;
        char *token = strtok(processed_line, " ");
        while (token) {
            args[argc++] = token;
            token = strtok(NULL, " ");
        }
        args[argc] = NULL;
        if (is_builtin_command(args[0])) {
            return execute_builtin(args);
        }
    }else if(strncmp(processed_line, ")", 1) == 0) {
        // 忽略 )
        return true;
    } else {
        // 执行外部命令
        return execute_command(processed_line);
    }
    return true;
}


// 解析 .batch 文件
bool execute_batch_file(const char *filename, int argc, char **args) {
    load_system_env(); // 加载系统环境变量
    for (int i = 0; i < argc && i < MAX_ARGS; ++i) {
        batch_args[i] = args[i];
        // printf("arg[%d]: %s\n", i, batch_args[i]);
    }
    batch_argc = argc;
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
