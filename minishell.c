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

/**
 * Mode for opening a file when reading. This mode is used in `fopen()` to
 * specify read-only access to the file.
 */
#define READ "r"

/**
 * Mode for opening a file when writing. This mode is used in `fopen()` to
 * specify write-only access to the file.
 */
#define WRITE "w"

void redirect(const char *filename, const char *MODE, FILE *file, int *fd, const int STD_FILENO, int *stdfd);

void restore(int *stdfd, const int STD_FILENO, int fd);

int main(void)
{
    char buffer[MAXIMUM_LINE_LENGTH];

    tline *line;
    int isEmpty, isInputRedirected, isOutputRedirected, isErrorRedirected;
    char *filename;

    int stderrfd, stdinfd, stdoutfd;
    FILE errorFile, inputFile, outputFile;
    int errfd, infd, outfd;

    pid_t pid;

    char **arguments;
    char *command;

    printf(PROMPT);
    while (fgets(buffer, MAXIMUM_LINE_LENGTH, stdin))
    {
        line = tokenize(buffer);

        isEmpty = line == NULL;

        if (isEmpty)
        {
            continue;
        }

        isErrorRedirected = line->redirect_error != NULL;
        isInputRedirected = line->redirect_input != NULL;
        isOutputRedirected = line->redirect_output != NULL;

        if (isErrorRedirected)
        {
            filename = line->redirect_error;
            redirect(filename, WRITE, &errorFile, &errfd, STDERR_FILENO, &stderrfd);
        }

        if (isInputRedirected)
        {
            filename = line->redirect_input;
            redirect(filename, READ, &inputFile, &infd, STDIN_FILENO, &stdinfd);
        }

        if (isOutputRedirected)
        {
            filename = line->redirect_output;
            redirect(filename, WRITE, &outputFile, &outfd, STDOUT_FILENO, &stdoutfd);
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

        if (isErrorRedirected)
        {
            restore(&stderrfd, STDERR_FILENO, errfd);
        }

        if (isInputRedirected)
        {
            restore(&stdinfd, STDIN_FILENO, infd);
        }

        if (isOutputRedirected)
        {
            restore(&stdoutfd, STDOUT_FILENO, outfd);
        }

        printf(PROMPT);
    }

    return 0;
}

/**
 * Redirect a file to a specified file descriptor.
 *
 * Open a file with the given filename and mode, and redirect it to the 
 * specified file descriptor using the `dup2()` system call. It also saves the
 * current file descriptor of `STD_FILENO` for later restoration.
 *
 * @param filename    The name of the file to be opened and redirected.
 * @param MODE        The mode for opening the file ("r" for read, "w" for
 *                    write).
 * @param file        A pointer to a `FILE` structure that will store the opened
 *                    file.
 * @param fd          A pointer to an integer that will store the file
 *                    descriptor of the opened file.
 * @param STD_FILENO  The file descriptor representing standard
 *                    input/output/error (e. g., `STDIN_FILENO`).
 * @param stdfd       A pointer to an integer that will store the original file
 *                    descriptor of `STD_FILENO` for later restoration.
 *
 * Note: This function prints an error message to `stderr` if it cannot open the
 * file.
 */
void redirect(const char *filename, const char *MODE, FILE *file, int *fd, const int STD_FILENO, int *stdfd)
{
    file = fopen(filename, MODE);
    if (file == NULL)
    {
        fprintf(stderr, "%s: Error. %s\n", filename, strerror(errno));
        exit(EXIT_FAILURE);
    }
    *fd = fileno(file);

    dup2(STD_FILENO, *stdfd);
    dup2(*fd, STD_FILENO);
}

/**
 * Restore the original file descriptor of `STD_FILENO` and closes the file.
 *
 * Restore the original file descriptor of `STD_FILENO` that was saved during
 * redirection using the `redirect()` function. It also closes the file 
 * associated with the given `FILE` pointer.
 *
 * @param stdfd       A pointer to the original file descriptor of `STD_FILENO`
 *                    saved during redirection.
 * @param STD_FILENO  The file descriptor representing standard input/output
 *                    (e.g., `STDIN_FILENO`).
 * @param file        A pointer to a `FILE` structure that represents the opened
 *                    file during redirection.
 */
void restore(int *stdfd, const int STD_FILENO, int fd)
{
    dup2(*stdfd, STD_FILENO);
    close(fd);
}
