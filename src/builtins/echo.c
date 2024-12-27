#include <stdio.h>
#include <string.h>
#include "builtintools.h"
#include <stdbool.h>

// 实现 ECHO 命令
bool builtin_echo(const char **args) {
    if (args == NULL || args[1] == NULL) {
        printf("\n"); // 如果没有参数，输出空行
        return true;
    }
    for (int i = 1; args[i] != NULL; i++) { // 从 args[1] 开始，跳过命令本身
        if (i > 1) {
            printf(" ");
        }
        printf("%s", args[i]);
    }
    printf("\n");
    return true;
}
