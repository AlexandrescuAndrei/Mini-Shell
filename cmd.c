// SPDX-License-Identifier: BSD-3-Clause

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmd.h"
#include "utils.h"

#define READ		0
#define WRITE		1
#define ERR_OUT     2
#define AUX         3


/**
 * Internal change-directory command.
 */
static bool shell_cd(word_t *dir)
{
	/* TODO: Execute cd. */
	int status = -1;
	char *target = get_word(dir);

	if (!target)
		target = getenv("HOME");
	if (target)
		status = chdir(target);
	free(target);
	return status;
}


/**
 * Internal exit/quit command.
 */
static int shell_exit(void)
{
	/* TODO: Execute exit/quit. */
	close(READ);
	close(WRITE);
	close(ERR_OUT);
	close(AUX);
	return -100;
}


/**
 * Parse a simple command (internal, environment variable assignment,
 * external command).
 */
static int parse_simple(simple_command_t *s, int level, command_t *father)
{
	/* TODO: Sanity checks. */

	/* TODO: If builtin command, execute the command. */

	/* TODO: If variable assignment, execute the assignment and return
	 * the exit status.
	 */

	/* TODO: If external command:
	 *   1. Fork new process
	 *     2c. Perform redirections in child
	 *     3c. Load executable in child
	 *   2. Wait for child
	 *   3. Return exit status
	 */
	char *commandName = get_word(s->verb);

	if (!s || level < 0)
		return -100;

	if (strcmp(commandName, "quit") == 0 || strcmp(commandName, "exit") == 0) {
		free(commandName);
		return shell_exit();
	}

	if (strcmp(commandName, "cd") == 0) {
		free(commandName);
		int outFD = -1, errFD = -1, combinedFD = -1;
		char *outFile = get_word(s->out);
		char *errFile = get_word(s->err);

		if (outFile && errFile && strcmp(outFile, errFile) == 0) {
			combinedFD = open(outFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (combinedFD < 0) {
				close(combinedFD);
				free(outFile);
				free(errFile);
				exit(EXIT_FAILURE);
			}
		} else {
			if (outFile) {
				int flags = (s->io_flags == IO_OUT_APPEND) ? O_APPEND : O_TRUNC;

				outFD = open(outFile, O_WRONLY | O_CREAT | flags, 0644);
				if (outFD < 0) {
					close(outFD);
					free(outFile);
					free(errFile);
					exit(EXIT_FAILURE);
				}
			}
			if (errFile) {
				int flags = (s->io_flags == IO_ERR_APPEND) ? O_APPEND : O_TRUNC;

				errFD = open(errFile, O_WRONLY | O_CREAT | flags, 0644);
				if (errFD < 0) {
					close(errFD);
					free(outFile);
					free(errFile);
					exit(EXIT_FAILURE);
				}
			}
		}

		int status = shell_cd(s->params);

		if (status < 0) {
			if (errFD >= 0) {
				FILE *errorFile = fdopen(errFD, "w");

				fprintf(errorFile, "Failed to change directory\n");
				fclose(errorFile);
			} else {
				fprintf(stderr, "Failed to change directory\n");
			}
		}

		free(outFile);
		free(errFile);

		if (outFD >= 0)
			close(outFD);
		if (errFD >= 0)
			close(errFD);
		if (combinedFD >= 0)
			close(combinedFD);
		free(commandName);
		return status;
	}

	if (strchr(commandName, '=')) {
		char *varName = (char *)s->verb->string;
		char *varValue = get_word(s->verb->next_part->next_part);
		int result = setenv(varName, varValue, 1);

		free(commandName);
		free(varValue);
		return result;
	}

	int status, argCount = 0;

	char **args = get_argv(s, &argCount);
	pid_t pid = fork();

	if (pid < 0) {
		perror("Fork failed");
		abort();
	} else if (pid == 0) {
		int *inFD = malloc(sizeof(int));
		int *outFD = malloc(sizeof(int));
		int *errFD = malloc(sizeof(int));
		int *combinedFD = malloc(sizeof(int));
		*inFD = -1;
		*outFD = -1;
		*errFD = -1;
		*combinedFD = -1;
		char *inFile = get_word(s->in);
		char *outFile = get_word(s->out);
		char *errFile = get_word(s->err);

		if (inFile) {
			*inFD = open(inFile, O_RDONLY);
			if (*inFD < 0 || dup2(*inFD, READ) < 0) {
				close(*inFD);
				free(inFile);
				free(outFile);
				free(errFile);
				exit(EXIT_FAILURE);
			}
		}

		if (outFile && errFile && strcmp(outFile, errFile) == 0) {
			*combinedFD = open(outFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (*combinedFD < 0 || dup2(*combinedFD, WRITE) < 0 || dup2(*combinedFD, ERR_OUT) < 0) {
				close(*combinedFD);
				free(inFile);
				free(outFile);
				free(errFile);
				exit(EXIT_FAILURE);
			}
		} else {
			if (outFile) {
				int flags = (s->io_flags == IO_OUT_APPEND) ? O_APPEND : O_TRUNC;
				*outFD = open(outFile, O_WRONLY | O_CREAT | flags, 0644);
				if (*outFD < 0 || dup2(*outFD, WRITE) < 0) {
					close(*outFD);
					free(inFile);
					free(outFile);
					free(errFile);
					exit(EXIT_FAILURE);
				}
			}
			if (errFile) {
				int flags = (s->io_flags == IO_ERR_APPEND) ? O_APPEND : O_TRUNC;
				*errFD = open(errFile, O_WRONLY | O_CREAT | flags, 0644);
				if (*errFD < 0 || dup2(*errFD, ERR_OUT) < 0) {
					close(*errFD);
					free(inFile);
					free(outFile);
					free(errFile);
					exit(EXIT_FAILURE);
				}
			}
		}
		free(inFile);
		free(outFile);
		free(errFile);

		execvp(commandName, args);

		if (*inFD >= 0)
			close(*inFD);
		if (*outFD >= 0)
			close(*outFD);
		if (*errFD >= 0)
			close(*errFD);
		if (*combinedFD >= 0)
			close(*combinedFD);

		fprintf(stderr, "%s '%s'\n", "Execution failed for", commandName);
		abort();
	} else {
		waitpid(pid, &status, 0);
		for (int i = 0; i < argCount; i++)
			free(args[i]);
		free(args);
		free(commandName);
		if (WIFEXITED(status))
			return WEXITSTATUS(status);
		return 1;
	}
	return 0;
}


/**
 * Process two commands in parallel, by creating two children.
 */
static bool run_in_parallel(command_t *cmd1, command_t *cmd2, int level,
		command_t *father)
{
	/* TODO: Execute cmd1 and cmd2 simultaneously. */

	pid_t child1 = fork();

	if (child1 < 0) {
		perror("Fork failed for child1");
		abort();
	} else if (child1 == 0) {
		exit(parse_command(cmd1, level, father));
	} else {
		pid_t child2 = fork();

		if (child2 < 0) {
			perror("Fork failed for child2");
			abort();
		} else if (child2 == 0) {
			exit(parse_command(cmd2, level, father));
		} else {
			int s_first, s_second;

			waitpid(child1, &s_first, 0);
			waitpid(child2, &s_second, 0);

			if (WIFEXITED(s_first))
				if (WIFEXITED(s_second)) {
					if (WEXITSTATUS(s_first) == 0 && WEXITSTATUS(s_second) == 0)
						return 0;
					else
						return 1;
				}
		}
	}
	return 0;
}


/**
 * Run commands by creating an anonymous pipe (cmd1 | cmd2).
 */
static bool run_on_pipe(command_t *cmd1, command_t *cmd2, int level,
		command_t *father)
{
	/* TODO: Redirect the output of cmd1 to the input of cmd2. */

	int pipefds[2];

	if (pipe(pipefds) < 0) {
		perror("Pipe creation failed");
		abort();
	}

	pid_t child1 = fork();

	if (child1 < 0) {
		perror("Fork failed for pipe child1");
		abort();
	} else if (child1 == 0) {
		close(pipefds[0]);
		dup2(pipefds[1], WRITE);
		close(pipefds[1]);

		exit(parse_command(cmd1, level, father));
	} else {
		pid_t child2 = fork();

		if (child2 < 0) {
			perror("Fork failed for pipe child2");
			abort();
		} else if (child2 == 0) {
			close(pipefds[1]);
			dup2(pipefds[0], READ);
			close(pipefds[0]);

			exit(parse_command(cmd2, level, father));
		} else {
			close(pipefds[0]);
			close(pipefds[1]);

			int status;

			waitpid(child2, &status, 0);

			if (WIFEXITED(status))
				return WEXITSTATUS(status);
			return 1;
		}
	}
	return 0;
}


/**
 * Parse and execute a command.
 */
int parse_command(command_t *c, int level, command_t *father)
{
	/* TODO: sanity checks */

	if (!c || level < 0)
		return -100;

	if (c->op == OP_NONE)
		return parse_simple(c->scmd, level + 1, father);

	if (c->op == OP_CONDITIONAL_ZERO) {
		if (!parse_command(c->cmd1, level + 1, father))
			return parse_command(c->cmd2, level + 1, father);
		return parse_command(c->cmd1, level + 1, father);
	}

	if (c->op == OP_PIPE)
		return run_on_pipe(c->cmd1, c->cmd2, level + 1, father);

	if (c->op == OP_SEQUENTIAL)
		return parse_command(c->cmd1, level + 1, father) | parse_command(c->cmd2, level + 1, father);

	if (c->op == OP_PARALLEL)
		return run_in_parallel(c->cmd1, c->cmd2, level + 1, father);

	if (c->op == OP_CONDITIONAL_NZERO) {
		if (parse_command(c->cmd1, level + 1, father))
			return parse_command(c->cmd2, level + 1, father);
		return parse_command(c->cmd1, level + 1, father);
	}

	return -100;
}
