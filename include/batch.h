#ifndef BATCH_PARSER_H
#define BATCH_PARSER_H

#include <stdbool.h>

// 执行 .batch 文件
// 参数: filename - .batch 文件路径
// 返回: 成功执行返回 true，失败返回 false
bool execute_batch_file(const char *filename);

// 执行批处理文件中的单行命令
bool execute_single_command(const char *line, FILE *current_file_pointer);

#endif // BATCH_PARSER_H
