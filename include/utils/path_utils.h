#ifndef PATH_UTILS_H
#define PATH_UTILS_H

/**
 * Searches for an executable in the system PATH.
 *
 * @param command The name of the executable to search for.
 * @return A dynamically allocated string containing the full path to the executable,
 *         or NULL if the executable is not found. The caller is responsible for freeing the memory.
 */
char *find_executable_in_path(const char *command);

#endif // PATH_UTILS_H
