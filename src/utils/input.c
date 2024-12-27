#include "utils/input.h"
#include "config.h"
#include "builtintools.h"

bool console_input(char *buffer){
    char ch, next_ch;
    int index = 0;  // 缓冲区当前索引
    // 使用getchar()逐个读取字符
    while (1) {
        ch = getchar();  // 获取一个字符

        // 处理转义字符 '\\n'，替换为换行符
        if (ch == '\\') {
            next_ch = getchar();  // 获取下一个字符
            if (next_ch == 'n') {
                buffer[index++] = '\n';  // 将 '\\n' 替换为换行符
            } else if(next_ch == '\n') {
                // 如果是 '\\\n'，表示继续输入，不做处理
            }
             else {
                buffer[index++] = '\\';  // 如果不是 'n'，就将 '\\' 放入缓冲区
                buffer[index++] = next_ch;  // 将下一个字符放入缓冲区
            }
        } else if (ch == '\n') {
            // 如果是换行符，结束输入并加入缓冲区
            buffer[index] = '\0';  // 在缓冲区末尾添加字符串结束符
            break;  // 退出循环
        } else if (ch == EOF) {
            // 如果是EOF，结束输入并退出shell
            buffer[index] = '\0';  // 在缓冲区末尾添加字符串结束符
            exit_shell();  // 退出shell
            break;  // 退出循环
        } 
        else{
            buffer[index++] = ch;  // 将字符存入缓冲区
        }

        // 检查缓冲区是否已满
        if (index >= INPUT_BUFFER_SIZE - 1) {
            buffer[index] = '\0';  // 在缓冲区末尾添加字符串结束符
            break;  // 退出循环
        }
    }
    return true;
}