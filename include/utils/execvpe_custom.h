// include/execvpe_custom.h

#ifndef EXECVPE_CUSTOM_H
#define EXECVPE_CUSTOM_H

/**
 * @brief 自定义实现 execvpe 函数
 *
 * @param file 要执行的文件路径或可执行文件名
 * @param argv 参数数组
 * @param envp 环境变量数组
 * @return int 如果成功，函数不会返回；如果失败，返回 -1 并设置 errno
 */
int execvpe_custom(const char *file, char *const argv[], char *const envp[]);

#endif // EXECVPE_CUSTOM_H
