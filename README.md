# Unix-Like Mini Shell in C

A small Unix-like command shell implemented in **C** using low-level process and file-descriptor operations.

The shell can execute external programs, handle built-in commands, work with environment variables, redirect standard input and output, connect commands through pipes, and execute more complex command combinations.

The main purpose of the project was to better understand how command shells interact with the operating system through processes, file descriptors, pipes, environment variables, and system calls.

## Command Execution

Simple external commands are executed by creating a child process with `fork()` and replacing the child process image using `execvp()`.

The parent process waits for the command to finish using `waitpid()` and returns the exit status of the executed program.

Command arguments are dynamically constructed from the parsed command structure before being passed to `execvp()`.

The shell also expands environment variables while building command words and arguments.

## Built-in Commands and Environment Variables

The shell implements several operations directly without launching an external executable.

The `cd` command changes the current working directory using `chdir()`. If no directory is provided, the shell uses the `HOME` environment variable.

The `exit` and `quit` commands terminate the shell.

Environment variable assignments are also supported and are applied using `setenv()`, allowing variables to be created or updated inside the current shell process.

## I/O Redirection

External commands can redirect their standard input, standard output, and standard error streams.

The implementation opens the required files and uses `dup2()` to replace the corresponding standard file descriptors before executing the command.

Both normal output redirection and append mode are supported.

Standard output and standard error can also be redirected to the same destination when required.

This part of the project works directly with Unix file descriptors and functions such as `open()`, `close()`, and `dup2()`.

## Pipes and Command Composition

The shell supports connecting commands through anonymous pipes.

For a pipeline, two child processes are created. The standard output of the first command is connected to the write end of the pipe, while the standard input of the second command is connected to the read end.

The shell also supports more complex command execution through parsed command operators.

Commands can be executed sequentially, in parallel, or conditionally depending on the exit status of a previous command.

Parallel execution is implemented by creating separate child processes for both commands and waiting for them to finish.

Conditional execution uses the return status of the first command to decide whether the second command should be executed.

## Interactive Shell

The application runs as an interactive shell and continuously reads commands from standard input.

Each command line is parsed into an internal command structure and then passed to the command execution logic.

The shell continues accepting commands until the user executes `exit` or `quit`, or until the input stream is closed.

Input is read dynamically, allowing command lines larger than a single fixed-size buffer.

## Project Structure

- `main.c` — interactive shell loop, command reading, and parser integration
- `cmd.c` / `cmd.h` — command execution, built-ins, redirections, pipes, and command operators
- `utils.c` / `utils.h` — command argument construction and environment-variable expansion
- `Makefile` — build configuration

The project also relies on an external parser referenced through the `UTIL_PATH` variable in the Makefile.

## Build and Run

The project uses **GCC** and **GNU Make**.

Running `make` builds the parser dependency and then creates the `mini-shell` executable.

The shell can then be started by running `./mini-shell`.

Once started, it displays a prompt and waits for commands to be entered interactively.

## Technologies and Concepts

- C
- Linux / Unix process model
- `fork()` and `execvp()`
- `waitpid()`
- Anonymous pipes
- File descriptors
- `dup2()`
- I/O redirection
- Environment variables
- Process synchronization
- Sequential execution
- Parallel execution
- Conditional command execution
- GCC
- GNU Make
