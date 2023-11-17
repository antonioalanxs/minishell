#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "parser.h"

/**
 * Maximum number of characters per input line. Adjusting this value allows
 * controlling the maximum length of input lines.
 */
#define MAXIMUM_LINE_LENGTH 1024

/**
 * Text string for the command line prompt when waiting for user input.
 */
#define PROMPT "msh> "

/**
 * Error result for fork operation.
 */
#define FORK_ERROR -1

/**
 * Result code for the child process in a fork operation.
 */
#define FORK_CHILD 0

/**
 * Argument to the `exit()` function to indicate that the program has executed
 * successfully without errors.
 */
#define EXIT_SUCCESS 0

/**
 * Argument to the `exit()` function to indicate that the program encountered
 * an error or failure during execution signaling to the operating system that
 * an issue occurred.
 */
#define EXIT_FAILURE 1

int enter(tline *line);

int main(void)
{
    char buffer[MAXIMUM_LINE_LENGTH];
    tline *line;
    pid_t pid;
    char **arguments;
    char *command;

    printf(PROMPT);
    while (fgets(buffer, MAXIMUM_LINE_LENGTH, stdin))
    {
        line = tokenize(buffer);

        if (enter(line))
        {
            continue;
        }

        pid = fork();

        if (pid == FORK_ERROR)
        {
            fprintf(stderr, "%s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        else if (pid == FORK_CHILD)
        {
            arguments = line->commands[0].argv;
            command = arguments[0];

            execvp(command, arguments);

            fprintf(stderr, "%s: Command not found\n", command);
            exit(EXIT_FAILURE);
        }
        else
        {
            wait(NULL);
        }

        printf(PROMPT);
    }

    return 0;
}

/**
 * Check whether the provided line pointer is NULL, indicating that the user
 * has pressed Enter without entering any text, resulting in an empty input
 * line.
 *
 * @param line Pointer to the structure representing the input line.
 * @return 1 if the input line is empty (Enter pressed), 0 otherwise.
 */
int enter(tline *line)
{
    return line == NULL;
}
