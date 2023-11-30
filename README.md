# minishell

## Table of Contents

1. [Overview](#overview)
2. [Installation](#installation)
   - [Prerequisites](#prerequisites)
   - [Cloning the Repository](#cloning-the-repository)
   - [Usage](#usage)
3. [Features](#features)
   - [Command Execution](#command-execution)
   - [Input and Output Redirection](#input-and-output-redirection)
   - [Background Execution](#background-execution)
   - [Internal Commands](#internal-commands)
     - [`cd`](#cd-command)
     - [`umask`](#umask-command)
     - [`exit`](#exit-command)
     - [`jobs`](#jobs-command)
     - [`fg`](#fg-command)
   - [Signal Handling](#signal-handling)
4. [Acknowledgments](#acknowledgments)
5. [License](#license)

## Overview

Simple shell program designed to interpret and execute commands entered by the user. It supports the execution of external commands, input and output redirection, command piping, background execution, and various internal commands such as `cd`, `umask`, `exit`, `jobs`, and `fg`.

## Installation

### Prerequisites

Ensure that the GCC compiler is installed.

### Cloning the Repository

```shell
git clone https://github.com/antonioalanxs/minishell
cd minishell
```

### Usage

Compile the project and execute the `minishell` executable:

```shell
chmod u+x ./compile.sh
./compile.sh
chmod u+x ./minishell
./minishell
```

The minishell is now running. To exit the shell, simply execute the `exit` command.

## Features

### Command Execution

minishell can execute external commands and handle command piping, allowing users to chain commands together using the `|` character.

```shell
msh> ls | grep lib | wc -l
```

### Input and Output Redirection

Users can redirect command input, output, and errors using `<`, `>`, and `>&` respectively.

```shell
msh> head -3 < input.txt > output.txt &>error.txt
```

### Background Execution

Commands can be sent to the background using the `&` character, enabling users to continue using the shell while a command is running.

```shell
msh> sleep 20 &
[1] 4449
msh> sleep 500 &
[2] 6754
msh> sleep 30 &
[3] 7643
```

### Internal Commands

#### `cd` Command

Allows users to change the current working directory. If no directory is specified, changes the current working directory to `$HOME`.

```shell
msh> cd
msh> pwd
/home/user
msh> cd ./dir/dir2/dir3
msh> pwd
/home/user/dir/dir2/dir3
msh> cd ..
msh> pwd
/home/user/dir/dir2
```

#### `umask` Command

Enables users to change the system mask for file creation permissions.

```shell
msh> umask 0022
0022
msh> umask
0022
```

#### `exit` Command

Terminates all running processes associated with active jobs and exits the minishell.

#### `jobs` Command

Displays tasks running in the background.

```shell
msh> jobs
[2] Running       sleep 500 &
[3] Running       sleep 30 &
[1] Done          sleep 20 &
```

#### `fg` Command

Brings background tasks to the foreground.

```shell
msh> fg
sleep 500 &
```

```shell
msh> fg 3
sleep 30 &
```

### Signal Handling

Handles the `SIGNINT` (Ctrl-C) signal gracefully, ensuring that pressing it does not close the shell. If a command is running in the foreground, pressing Ctrl-C cancels its execution.

## Acknowledgments

This minishell project is inspired by the bash shell, and understanding its functionality is enhanced by referring to the [bash manual](https://www.gnu.org/software/bash/manual/bash.html).

## License

This project is licensed under the **Apache License 2.0** - see the [LICENSE](LICENSE) file for details.
