#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "utils/input.h"
#include "shell.h"
#include "batch.h"

int main(int argc, char *argv[]) {
    if (argc == 1) shell_loop();
    else {
        printf("Execute batch file: %s, argc:%d\n", argv[1], argc-2);
        // for (int i = 2; i < argc; i++) {
        //     printf("argv[%d]: %s\n", i, argv[i]);
        // }
        execute_batch_file(argv[1], argc-2, argv+2);
    }
    return 0;
}
