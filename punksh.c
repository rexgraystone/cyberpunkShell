#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
 
#include <sys/types.h>
#include <sys/wait.h>
 
/* The array below will hold the arguments: args[0] is the command. */
static char* args[512];
pid_t pid;
int command_pipe[2];
 
#define READ  0
#define WRITE 1
#define MAX_PATH 0

int errno;

char lastdir[MAX_PATH];

/**
 * @brief 
 * 
 * @param input 
 * @param first 
 * @param last 
 * @return int 
 */
static int command(int input, int first, int last)
{
	int pipettes[2];
 
	/* Invoke pipe */
	pipe(pipettes);	
	pid = fork();
 
	/*
	 SCHEME:
	 	STDIN --> O --> O --> O --> STDOUT
	*/
 
	if (pid == 0) {
		if (first == 1 && last == 0 && input == 0) {
			// First command
			dup2( pipettes[WRITE], STDOUT_FILENO );
		} else if (first == 0 && last == 0 && input != 0) {
			// Middle command
			dup2(input, STDIN_FILENO);
			dup2(pipettes[WRITE], STDOUT_FILENO);
		} else {
			// Last command
			dup2( input, STDIN_FILENO );
		}
 
		if (execvp( args[0], args) == -1)
			_exit(EXIT_FAILURE); // If child fails
	}
 
	if (input != 0) 
		close(input);
 
	// Nothing more needs to be written
	close(pipettes[WRITE]);
 
	// If it's the last command, nothing more needs to be read
	if (last == 1)
		close(pipettes[READ]);
 
	return pipettes[READ];
}
 
/** @brief Final cleanup, 'wait' for processes to terminate.
 *  @param n Number of times 'command' was invoked.
 */
static void cleanup(int n)
{
	int i;
	for (i = 0; i < n; ++i) 
		wait(NULL); 
}
 
static int run(char* cmd, int input, int first, int last);
static char line[1024];
static int n = 0; /* number of calls to 'command' */
 
int main()
{
	printf("\n\t\t\tpunkShell: High time you chromed up.\n");
	printf("\t\t\tpunkShell: Type 'flatline' or 'exit' to exit.\n");
	while (1) {
		/* Print the command prompt */
		printf("€$ ");
		fflush(NULL);
 
		/* Read a command line */
		if (!fgets(line, 1024, stdin)) 
			return 0;
 
		int input = 0;
		int first = 1;
 
		char* cmd = line;
		char* next = strchr(cmd, '|'); /* Find first '|' */
 
		while (next != NULL) {
			/* 'next' points to '|' */
			*next = '\0';
			input = run(cmd, input, first, 0);
 
			cmd = next + 1;
			next = strchr(cmd, '|'); /* Find next '|' */
			first = 0;
		}
		input = run(cmd, input, first, 1);
		cleanup(n);
		n = 0;
	}
	return 0;
}
 
static void split(char* cmd);

int exec_cd(char *arg) {
    char curdir[MAX_PATH];
    char path[MAX_PATH];

    if (getcwd(curdir, sizeof curdir)) {
        /* current directory might be unreachable: not an error */
        *curdir = '\0';
    }
    if (arg == NULL) {
        arg = getenv("HOME");
    }
    if (!strcmp(arg, "-")) {
        if (*lastdir == '\0') {
            fprintf(stderr, "no previous directory\n");
            return 1;
        }
        arg = lastdir;
    } else {
        /* this should be done on all words during the parse phase */
        if (*arg == '~') {
            if (arg[1] == '/' || arg[1] == '\0') {
                snprintf(path, sizeof path, "%s%s", getenv("HOME"), arg + 1);
                arg = path;
            } else {
                /* ~name should expand to the home directory of user with login `name` 
                   this can be implemented with getpwent() */
                fprintf(stderr, "syntax not supported: %s\n", arg);
                return 1;
            }
        }
    }
    if (chdir(arg)) {
        fprintf(stderr, "punksh: %s: %s\n", strerror(errno), path);
        return 1;
    }
    strcpy(lastdir, curdir);
    return 0;
}

static int run(char* cmd, int input, int first, int last)
{
	split(cmd);
	if (args[0] != NULL) {
		if ((strcmp(args[0], "flatline") == 0) || (strcmp(args[0], "exit") == 0))
			exit(0);
		if (strcmp(args[0], "cd") == 0) {
			exec_cd(args[1]);
			return 0;
		}
		n += 1;
		return command(input, first, last);
	}
	return 0;
}
 
static char* skipwhite(char* s)
{
	while (isspace(*s)) ++s;
	return s;
}
 
static void split(char* cmd)
{
	cmd = skipwhite(cmd);
	char* next = strchr(cmd, ' ');
	int i = 0;
 
	while(next != NULL) {
		next[0] = '\0';
		args[i] = cmd;
		++i;
		cmd = skipwhite(next + 1);
		next = strchr(cmd, ' ');
	}
 
	if (cmd[0] != '\0') {
		args[i] = cmd;
		next = strchr(cmd, '\n');
		next[0] = '\0';
		++i; 
	}
 
	args[i] = NULL;
}
