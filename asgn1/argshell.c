#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define PIPE  '|'
#define STDIN  0
#define STDOUT 1
#define STDERR 2
#define OUT   ">"
#define IN    "<"
#define AND   '&'
#define AMPER ">>"



extern char ** get_args();
int run_cmd(int, char**, int, int, int[2], int);
void chop_it_up(char**, int);
char* start_dir;
int trail_check(char**, int);
int str_contains(char*, char);

int main() {
	int         i;
	char **     args;

	start_dir = malloc(sizeof(char) * 512);
	start_dir = getcwd(start_dir, 512);
	if (!start_dir) { return -1; }

	while (1) 
	{
		printf ("Command ('exit' to quit): ");
		args = get_args();

		int start_index = 0;
		// parse arguments by looping until a ';' is found.
		for (i = 0; args[i] != NULL; i++) 
		{    
			if (args[i][0] == ';') 
			{
				// NULL teminated command to not continue
				args[i] = NULL;
				// chop it up will look for pipes and handle file descriptor table...
				chop_it_up(&args[start_index], i - start_index);
				// increment start index for the possible next command...
				start_index = (i + 1);
			}
		}
		
		if (start_index < i) 
		{
			// we didn't end on a semi-colon...
			// so pass the command to chop it up, in case their are pipes...
			chop_it_up(&args[start_index], i - start_index); 
		}
		if (args[0] == NULL) 
		{
			printf ("No arguments on line!\n");
		} 
	}
}


/**
	chop_it_up handles pipes
*/
void chop_it_up(char** a, int size) {
	// check for invalid pipes, either leading or trailing...
	if (a[size - 1][0] == PIPE || a[0][0] == PIPE) 
	{
		fprintf(stderr, "Error: invalid pipe location\n");
	}
	
	int cmd_start = 0; // start of the command(s)
	int cmd_length = 0; // length of command 
	int prev_out_pipe = 0; 
	int fd_pipe[2] = {0, 0}; // for redirection 

	for (int i = 0; i < size; i++) 
	{
		if (i == size - 1) 
		{
			pipe (fd_pipe);
			run_cmd (i - cmd_start + 1, &a[cmd_start], prev_out_pipe, STDOUT, fd_pipe, 0);
			cmd_start = i;
		} 
		else if (a[i][0] == PIPE) 
		{
			cmd_length = i - cmd_start;
			char *split_out_cmd[cmd_length + 1];
			for (int j = 0; j < cmd_length; j++) 
			{
				split_out_cmd[j] = a[cmd_start + j];
			}

			split_out_cmd[cmd_length] = NULL;
			cmd_start = i + 1;
			pipe (fd_pipe);

			if (strcmp(a[i], "|&") == 0) 
			{
				run_cmd (cmd_length, split_out_cmd, prev_out_pipe, fd_pipe[1], fd_pipe, 1);
			} else 
			{
				run_cmd (cmd_length, split_out_cmd, prev_out_pipe, fd_pipe[1], fd_pipe, 0);
			}
			prev_out_pipe = fd_pipe[0];
		}
	}
}

int run_cmd(int size, char** a, int fd_in, int fd_out, int pipes[2], int isErr) {
	// check for the two calls that aren't ran with fork() and execvp()...
	if (strcmp (a[0], "exit") == 0) 
	{
		_exit(0);
	} 
	else if (strcmp (a[0], "cd") == 0) 
	{
		// handle zero args...
		if (a[1] == NULL) 
		{
			// saved start_dir in main for the zerp argument case...
			chdir(start_dir);
			return (0);
		} 
		else 
		{
			int ret_val = chdir (a[1]);
			// check for invalid argument...
			if (ret_val < 0) 
			{
				perror (a[1]);
				return (-1);
			} 
			else 
			{
				// cd was successful...
				return (0);
			}
		}
	}

	// now, for the syscalls...
	int fd_redir;
	int exec_check = 0;
	// create a child process to run the syscall...
	pid_t pid = fork(); 	
	// child process has pid of 0...
	if (pid == 0) 
	{
		// fd_in and fd_out were handled by chop_it_up and passed to run_cmd...
		dup2(fd_in, STDIN);
		dup2(fd_out, STDOUT);
		if (isErr) 
		{ 
			dup2(fd_out, STDERR); 
		}

		close(pipes[0]);

		// check for trailing symbols
		if (!trail_check(a, size)) {
			fprintf (stderr, "%s\n", "Error: cmd ends in redirection symbol...");
			_exit(-1);
		}

		// loop through the args container and handle redirection...
		for (int i = 0; a[i] != NULL; i++) 
		{
			// if we have a '>', we need to create flags for redirection...
			if (str_contains(a[i], '>')) 
			{ 
				int flags = O_WRONLY | O_CREAT;
				
				if (strstr(a[i], AMPER) != NULL)
				{
					flags |= O_APPEND;
				}

				// open the file and check file descriptor...
				fd_redir = open(a[i + 1], flags, 0666);
				if (fd_redir < 0) 
				{
					perror(a[i + 1]);
					_exit(-1);
				}
				
				// change fd to open()'s return value for file redirection...
				dup2(fd_redir, STDOUT);

				// handle rediretion of standard error...	
				if (str_contains(a[i], AND)) 
				{
					dup2(fd_redir, STDERR);		
				}

				a[i] = NULL;

			}
			else if (strcmp(a[i], IN) == 0) 
			{
				fd_redir = open(a[i + 1], O_RDONLY);
				a[i] = NULL;

				if (fd_redir < 0) 
				{
					fprintf(stderr, "Failed to open %s for read...\n", a[i+1]);	
					perror(a[1]);
					_exit(-1);
				}
				dup2(fd_redir, STDIN);
			} 
		}

		// change image...
		exec_check = execvp(a[0], a);
		if (exec_check == -1) 
		{
			perror(a[0]);
			_exit(exec_check);
		}
	} 
	else 
	{
		int ret_val = 0;
		wait(&ret_val);	
		if (ret_val < 0) {
			printf("%d\n", ret_val);
			perror(a[0]);
		}
		close(pipes[1]);
	}
	return (0);
}

int str_contains (char* a, char character) {
	int i = 0; 
	while(a[i] != '\0') 
	{
		if (a[i] == character) 
		{
			return (1);
		}
		i++;
	}
	return (0);
}

int trail_check(char** a, int size) {
	return (strcmp(a[size - 1],  OUT) && strcmp(a[size - 1], IN) && strcmp(a[size - 1], ">>"));
}
