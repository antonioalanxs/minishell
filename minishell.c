#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

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
 * Result code for the child process in a fork operation.
 */
#define FORK_CHILD 0

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
#define FILE_READ "r"

/**
 * Mode for opening a file when writing. This mode is used in `fopen()` to
 * specify write-only access to the file.
 */
#define FILE_WRITE "w"

/**
 * Pipe in file descriptors array.
 */
#define PIPE 2

/**
 * Read end of a pipe.
 */
#define PIPE_READ 0

/**
 * Write end of a pipe.
 */
#define PIPE_WRITE 1

void store(int *stdinfd, int *stdoutfd, int *stderrfd);
void redirect(tline *line);
void auxiliarRedirect(char *filename, const char *MODE, const int STD_FILENO);
void executeCommand(const tline *line, int number);
void restore(const int stdinfd, const int stdoutfd, const int stderrfd);

int main(void)
{
    char buffer[MAXIMUM_LINE_LENGTH];
    tline *line;
    int stdinfd, stdoutfd, stderrfd;
    pid_t pid;
    int p[PIPE], p2[PIPE];
    int command;

    printf(PROMPT);
    while (fgets(buffer, MAXIMUM_LINE_LENGTH, stdin))
    {
        line = tokenize(buffer);

        if (line == NULL)
        {
            continue;
        }

        store(&stdinfd, &stdoutfd, &stderrfd);

        pipe(p);

        pid = fork();

        if (pid == FORK_CHILD)
        {
            redirect(line);

            close(p[PIPE_READ]);

            if (line->ncommands > 1)
            {
                dup2(p[PIPE_WRITE], STDOUT_FILENO);
            }
            close(p[PIPE_WRITE]);

            executeCommand(line, 0);
        }
        else
        {
            close(p[PIPE_WRITE]);

            wait(NULL);

            restore(stdinfd, stdoutfd, stderrfd);

            for (command = 1; command < line->ncommands; command++)
            {
                if (command % 2 == 0)
                {
                    pipe(p);
                }
                else
                {
                    pipe(p2);
                }

                pid = fork();

                if (pid == FORK_CHILD)
                {
                    redirect(line);

                    if (command % 2 == 0)
                    {
                        dup2(p2[PIPE_READ], STDIN_FILENO);
                        if (command != line->ncommands - 1)
                        {
                            dup2(p[PIPE_WRITE], STDOUT_FILENO);
                        }
                    }
                    else
                    {
                        dup2(p[PIPE_READ], STDIN_FILENO);
                        if (command != line->ncommands - 1)
                        {
                            dup2(p2[PIPE_WRITE], STDOUT_FILENO);
                        }
                    }
                    close(p[PIPE_READ]);
                    close(p[PIPE_WRITE]);
                    close(p2[PIPE_READ]);
                    close(p2[PIPE_WRITE]);

                    executeCommand(line, 1);
                }
                else
                {
                    if (command % 2 == 0)
                    {
                        dup2(STDIN_FILENO, p[PIPE_WRITE]);
                        close(p[PIPE_WRITE]);
                        close(p2[PIPE_READ]);
                        if (command == line->ncommands - 1)
                        {
                            close(p[PIPE_READ]);
                            close(p2[PIPE_WRITE]);
                        }
                    }
                    else
                    {
                        dup2(STDIN_FILENO, p2[PIPE_WRITE]);
                        close(p2[PIPE_WRITE]);
                        close(p[PIPE_READ]);
                        if (command == line->ncommands - 1)
                        {
                            close(p2[PIPE_READ]);
                            close(p[PIPE_WRITE]);
                        }
                    }

                    wait(NULL);
                }
            }

            restore(stdinfd, stdoutfd, stderrfd);
        }

        printf(PROMPT);
    }

    return 0;
}

/**
 * Store the standard input, output, and error file descriptors for later
 * restoration.
 *
 * @param stdinfd Pointer to the variable to store the original standard input
 * file descriptor.
 * @param stdoutfd Pointer to the variable to store the original standard
 * output file descriptor.
 * @param stderrfd Pointer to the variable to store the original standard error
 * file descriptor.
 */
void store(int *stdinfd, int *stdoutfd, int *stderrfd)
{
    dup2(STDERR_FILENO, *stderrfd);
    dup2(STDIN_FILENO, *stdinfd);
    dup2(STDOUT_FILENO, *stdoutfd);
}

/**
 * Redirect standard input, output, and error based on the information provided
 * in the given command line structure.
 *
 * @param line A pointer to a `tline` structure representing the command line.
 * @param stdinfd Pointer to the variable to store the original standard input
 * file descriptor.
 * @param stdoutfd Pointer to the variable to store the original standard
 * output file descriptor.
 * @param stderrfd Pointer to the variable to store the original standard error
 * file descriptor.
 */
void redirect(tline *line)
{
    if (line->redirect_error != NULL)
    {
        auxiliarRedirect(line->redirect_error, FILE_WRITE, STDERR_FILENO);
    }

    if (line->redirect_input != NULL)
    {
        auxiliarRedirect(line->redirect_input, FILE_READ, STDIN_FILENO);
    }

    if (line->redirect_output != NULL)
    {
        auxiliarRedirect(line->redirect_output, FILE_WRITE, STDOUT_FILENO);
    }
}

/**
 * Auxiliary function for redirecting a specific file descriptor based on the
 * given filename and mode.
 *
 * @param filename The name of the file to be used for redirection.
 * @param MODE The mode to be used in `fopen()` for opening the file (e.g.,
 * `FILE_READ`, `FILE_WRITE`).
 * @param STD_FILENO The standard file descriptor to be redirected (e.g.,
 * `STDIN_FILENO`, `STDOUT_FILENO`).
 */
void auxiliarRedirect(char *filename, const char *MODE, const int STD_FILENO)
{
    FILE *file;
    int fd;

    file = fopen(filename, MODE);
    if (file == NULL)
    {
        fprintf(stderr, "%s: Error. %s\n", filename, strerror(errno));
        exit(EXIT_FAILURE);
    }

    fd = fileno(file);

    dup2(fd, STD_FILENO);

    fclose(file);
    close(fd);
}

/**
 * Execute a command specified by the given command line structure.
 *
 * @param line A pointer to a `tline` structure representing the command line.
 * @param number The index of the command to be executed within the command
 * line.
 *
 * If the command execution fails, an error message is printed to `stderr`
 * indicating that the command was not found, and the program exits with a
 * failure status.
 */
void executeCommand(const tline *line, int number)
{
    char **arguments;
    char *command;

    arguments = line->commands[number].argv;
    command = arguments[0];

    execvp(command, arguments);

    fprintf(stderr, "%s: Command not found\n", command);
    exit(EXIT_FAILURE);
}

/**
 * Restore the original standard input, output, and error file descriptors.
 *
 * @param stdinfd The original standard input file descriptor.
 * @param stdoutfd The original standard output file descriptor.
 * @param stderrfd The original standard error file descriptor.
 */
void restore(const int stdinfd, const int stdoutfd, const int stderrfd)
{
    dup2(stderrfd, STDERR_FILENO);
    dup2(stdinfd, STDIN_FILENO);
    dup2(stdoutfd, STDOUT_FILENO);
}
