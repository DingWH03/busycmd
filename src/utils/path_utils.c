#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "utils/path_utils.h"

#define PATH_DELIMITER ":"

// Function to search for an executable in the system PATH
char *find_executable_in_path(const char *command) {
    if (!command || strlen(command) == 0) {
        return NULL;
    }

    // Check if command contains a path separator (e.g., ./command)
    if (strchr(command, '/') != NULL) {
        if (access(command, X_OK) == 0) {
            return strdup(command);
        } else {
            return NULL;
        }
    }

    // Get the PATH environment variable
    char *path_env = getenv("PATH");
    if (!path_env) {
        return NULL;
    }

    // Duplicate the PATH string for tokenization
    char *path_copy = strdup(path_env);
    if (!path_copy) {
        perror("strdup failed");
        return NULL;
    }

    char *token = strtok(path_copy, PATH_DELIMITER);
    while (token) {
        // Construct the full path for the command
        size_t full_path_size = strlen(token) + strlen(command) + 2;
        char *full_path = (char *)malloc(full_path_size);
        if (!full_path) {
            perror("malloc failed");
            free(path_copy);
            return NULL;
        }

        snprintf(full_path, full_path_size, "%s/%s", token, command);

        // Check if the file is executable
        if (access(full_path, X_OK) == 0) {
            free(path_copy);
            return full_path;
        }

        free(full_path);
        token = strtok(NULL, PATH_DELIMITER);
    }

    free(path_copy);
    return NULL;
}
