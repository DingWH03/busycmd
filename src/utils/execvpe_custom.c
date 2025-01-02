// src/execvpe_custom.c

#define _GNU_SOURCE
#include "utils/execvpe_custom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

/**
 * @brief 自定义实现 execvpe 函数
 *
 * @param file 要执行的文件路径或可执行文件名
 * @param argv 参数数组
 * @param envp 环境变量数组
 * @return int 如果成功，函数不会返回；如果失败，返回 -1 并设置 errno
 */
int execvpe_custom(const char *file, char *const argv[], char *const envp[]) {
    // 如果文件名中包含 '/', 直接尝试执行
    if (strchr(file, '/')) {
        execve(file, argv, envp);
        return -1; // execve 只有在失败时才会返回
    }

    // 获取 PATH 环境变量
    char *path_env = getenv("PATH");
    if (path_env == NULL) {
        errno = ENOENT;
        return -1;
    }

    // 复制 PATH 环境变量，因为 strtok 会修改字符串
    char *path_dup = strdup(path_env);
    if (path_dup == NULL) {
        // strdup 失败，设置 errno 并返回
        errno = ENOMEM;
        return -1;
    }

    char *saveptr = NULL;
    char *dir = strtok_r(path_dup, ":", &saveptr);
    while (dir != NULL) {
        // 构建完整的路径
        size_t len = strlen(dir) + 1 + strlen(file) + 1;
        char *full_path = malloc(len);
        if (full_path == NULL) {
            free(path_dup);
            errno = ENOMEM;
            return -1;
        }

        snprintf(full_path, len, "%s/%s", dir, file);

        // 检查文件是否存在且可执行
        struct stat sb;
        if (stat(full_path, &sb) == 0 && (sb.st_mode & S_IXUSR)) {
            // 尝试执行
            execve(full_path, argv, envp);
            // 如果 execve 返回，说明执行失败，继续尝试下一个路径
        }

        free(full_path);
        dir = strtok_r(NULL, ":", &saveptr);
    }

    // 没有找到可执行文件
    free(path_dup);
    errno = ENOENT;
    return -1;
}
