/*
 * A program to test the functionality of the parser library `parser.h`.
 *
 * This program reads input lines, tokenizes them using the functions provided
 * by the parser library, and prints information about the parsed commands.
 */


#include <stdio.h>

#include "parser.h"


int main(void)
{
	char buffer[1024];
	tline *line;
	int command_index, argument_index;

	// Loop continuously to read input lines until EOF (end-of-file)
	while (fgets(buffer, 1024, stdin))
	{
		line = tokenize(buffer);

		if (line == NULL)
		{
			continue;
		}

		if (line->redirect_input != NULL)
		{
			printf("input redirection: %s\n", line->redirect_input);
		}

		if (line->redirect_output != NULL)
		{
			printf("output redirection: %s\n", line->redirect_output);
		}

		if (line->redirect_error != NULL)
		{
			printf("error redirection: %s\n", line->redirect_error);
		}

		if (line->background)
		{
			printf("command to be executed in the background\n");
		}

		// Loop through each command in the input line
		for (command_index = 0; command_index < line->ncommands; command_index++)
		{
			printf("command %d (%s):\n", command_index, line->commands[command_index].filename);

			// Loop through each argument of the command
			for (argument_index = 0; argument_index < line->commands[command_index].argc; argument_index++)
			{
				printf("  argument %d: %s\n", argument_index, line->commands[command_index].argv[argument_index]);
			}
		}
	}

	return 0;
}
