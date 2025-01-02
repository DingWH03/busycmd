#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "utils/input.h"
#include "shell.h"
#include "batch.h"

int main(int argc, char *argv[]) {
    if (argc == 1) shell_loop();
    else if (argc == 2){
        printf("Excute batch file: %s\n", argv[1]);
        execute_batch_file(argv[1]);
    }
    else
    printf("Usage: %s [batchfile]\n", argv[0]);
    return 0;
}
