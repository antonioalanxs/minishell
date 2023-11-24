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

/**
 * Environment variable representing the user's home directory.
 */
#define HOME "HOME"

/**
 * Index representing the command part of an argument array.
 */
#define COMMAND 0

/**
 * Index representing the directory part of an argument array.
 */
#define DIRECTORY 1

void store(int *stdinfd, int *stdoutfd, int *stderrfd);
void redirect(tline *line);
void auxiliarRedirect(char *filename, const char *MODE, const int STD_FILENO);
void run(const tline *line, int number);
void restore(const int stdinfd, const int stdoutfd, const int stderrfd);
void executeExternalCommands(tline *line);
void mshcd(char *directory);
void mshexit(int commands);

int main(void)
{
    char buffer[MAXIMUM_LINE_LENGTH];
    tline *line;
    char **firstCommandArguments;

    printf(PROMPT);
    while (fgets(buffer, MAXIMUM_LINE_LENGTH, stdin))
    {
        line = tokenize(buffer);

        if (line == NULL)
        {
            continue;
        }

        firstCommandArguments = line->commands[0].argv;

        if (strcmp(firstCommandArguments[COMMAND], "cd") == 0)
        {
            mshcd(firstCommandArguments[DIRECTORY]);
        }
        else if (strcmp(firstCommandArguments[COMMAND], "exit") == 0)
        {
            mshexit(line->ncommands);
        }
        else
        {
            executeExternalCommands(line);
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
    }

    fd = fileno(file);

    dup2(fd, STD_FILENO);

    fclose(file);
    close(fd);
}

/**
 * Run a command specified by the given command line structure.
 *
 * @param line A pointer to a `tline` structure representing the command line.
 * @param number The index of the command to be ran within the command line.
 *
 * If the command execution fails, an error message is printed to `stderr`
 * indicating that was not found, and the program exits with a failure status.
 */
void run(const tline *line, int number)
{
    char **arguments;
    char *command;

    arguments = line->commands[number].argv;
    command = arguments[COMMAND];

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

/**
 * Executes a series of commands specified in the given command line structure.
 *
 * @param line A data structure representing a command line with multiple
 * commands.
 *
 * This function takes a `tline` command line structure as input and executes
 * the commands sequentially ensuring proper communication between consecutive
 * commands, managing the flow of input and output through pipes.
 *
 * Example:
 *   tline *line = tokenize("ls -l | grep .txt | wc -l");
 *   executeExternalCommands(line);
 *
 * Note:
 *   This function relies on the `parser.h` library and auxiliary functions
 *   like `store`, `redirect`, `run`, `restore`, and assumes the existence of
 *   constants like `PIPE_READ`, `PIPE_WRITE`, or `FORK_CHILD`.
 */
void executeExternalCommands(tline *line)
{
    int stdinfd, stdoutfd, stderrfd;
    int commands, command;
    int next, even, last;
    pid_t pid;
    int p[PIPE], p2[PIPE];

    store(&stdinfd, &stdoutfd, &stderrfd);

    commands = line->ncommands;
    next = commands > 1;

    if (next)
    {
        pipe(p);
    }

    pid = fork();

    if (pid == FORK_CHILD)
    {
        redirect(line);

        if (next)
        {
            // Does not receive input from anyone
            close(p[PIPE_READ]);

            // Pipe `stdout` output for next command input
            dup2(p[PIPE_WRITE], STDOUT_FILENO);
            close(p[PIPE_WRITE]);
        }

        run(line, 0);
    }
    else
    {
        // Only reads from pipe to provide input for next command
        close(p[PIPE_WRITE]);

        // Clears `stdout` and `stdin` in case there are following commands
        restore(stdinfd, stdoutfd, stderrfd);

        wait(NULL);

        for (command = 1; next && command < commands; command++)
        {
            even = command % 2 == 0;
            last = command == commands - 1;

            // Restore the child process writing pipe to prevent errors
            if (even)
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

                // Reads from one pipe and writes to another based on parity
                if (even)
                {
                    dup2(p2[PIPE_READ], STDIN_FILENO);
                    if (!last)
                    {
                        dup2(p[PIPE_WRITE], STDOUT_FILENO);
                    }
                }
                else
                {
                    dup2(p[PIPE_READ], STDIN_FILENO);
                    if (!last)
                    {
                        dup2(p2[PIPE_WRITE], STDOUT_FILENO);
                    }
                }
                close(p[PIPE_READ]);
                close(p[PIPE_WRITE]);
                close(p2[PIPE_READ]);
                close(p2[PIPE_WRITE]);

                run(line, command);
            }
            else
            {
                if (even)
                {
                    dup2(STDIN_FILENO, p[PIPE_WRITE]);
                    close(p[PIPE_WRITE]);

                    // Closes unnecessary pipes for the next odd child
                    close(p2[PIPE_READ]);
                    if (last)
                    {
                        close(p[PIPE_READ]);
                        close(p2[PIPE_WRITE]);
                    }
                }
                else
                {
                    dup2(STDIN_FILENO, p2[PIPE_WRITE]);
                    close(p2[PIPE_WRITE]);

                    // Closes unnecessary pipes for the next even child
                    close(p[PIPE_READ]);
                    if (last)
                    {
                        close(p2[PIPE_READ]);
                        close(p[PIPE_WRITE]);
                    }
                }

                wait(NULL);
            }
        }

        // Finish by cleaning `stdout` and `stdin` again for next iteration
        restore(stdinfd, stdoutfd, stderrfd);
    }
}

/**
 * Changes the current working directory.
 *
 * Changes the current working directory to the specified directory. If no
 * directory is provided (NULL), it changes to the HOME directory.
 *
 * @param directory The path of the target directory. If NULL, changes to the
 * HOME directory.
 */
void mshcd(char *directory)
{
    if (directory == NULL)
    {
        chdir(getenv(HOME));
    }
    else
    {
        chdir(directory);
    }
}

/**
 * Terminate the program, ensuring that all child processes spawned during its
 * execution have completed. It iterates over the specified number of commands,
 * waiting for each child process to finish before exiting the program with a
 * success status code.
 *
 * @param commands The number of commands or child processes spawned during the
 * execution of the program.
 */
void mshexit(int commands)
{
    int _;

    for (_ = 0; _ < commands; _++)
    {
        wait(NULL);
    }

    exit(EXIT_SUCCESS);
}
