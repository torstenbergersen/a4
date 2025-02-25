/*
Program name: bergest_assignment4.c (smallsh)
Author: Torsten Bergersen
Smallsh provides a prompt for running commands, handles blank lines and comments, executes 3 commands (exit, cd, and status) via code built into shell, execustes other commands by creating new processes, supports input and output redirection, supports running commands in foreground and background processes, and implements two custom handlers for signals SIGINT and SIGTSTP.
*/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>

#define INPUT_LENGTH 2048
#define MAX_ARGS		 512
#define MAX_BG_PROCESSES 100   // maximum background processes to track

// array to store background process ids and a counter for them
pid_t bg_pids[MAX_BG_PROCESSES];
int bg_pids_count = 0;

// flag for foreground-only mode; set by the SIGTSTP handler
volatile sig_atomic_t foreground_only = 0;

// sigaction structures for SIGINT and SIGTSTP handling
struct sigaction SIGINT_action = {0};
struct sigaction SIGTSTP_action = {0};

// structure representing a parsed command line
struct command_line {
	char *argv[MAX_ARGS + 1];
	int argc;
	char *input_file;
	char *output_file;
	bool is_bg;
};

// sigtstp handler to toggle foreground-only mode
void handle_SIGTSTP(int signo) {
    if (foreground_only == 0) {
        foreground_only = 1;
        char* message = "\nEntering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, message, strlen(message));
    } else {
        foreground_only = 0;
        char* message = "\nExiting foreground-only mode\n";
        write(STDOUT_FILENO, message, strlen(message));
    }
}
// parse_input: reads a line from stdin and tokenizes it into a command_line struct
// main logic taken from assignment starter code
struct command_line *parse_input() {
	char input[INPUT_LENGTH];
	struct command_line *curr_command = (struct command_line *) calloc(1, sizeof(struct command_line));

	// Get input
	printf(": ");
	fflush(stdout);
	fgets(input, INPUT_LENGTH, stdin);

	// if the line is a comment or blank, mark it as empty
	if (input[0] == '#' || input[0] == '\n') {
        curr_command->argv[0] = NULL;
        return curr_command;
    }

	// Tokenize the input
	char *token = strtok(input, " \n");
	while(token){
		if(!strcmp(token,"<")){
			curr_command->input_file = strdup(strtok(NULL," \n"));
		} else if(!strcmp(token,">")){
			curr_command->output_file = strdup(strtok(NULL," \n"));
		} else if(!strcmp(token,"&")){
			curr_command->is_bg = true;
		} else{
			curr_command->argv[curr_command->argc++] = strdup(token);
		}
		token=strtok(NULL," \n");
	}
	return curr_command;
}

// execute_command: forks and runs the given command with redirection as needed
int execute_command(struct command_line *curr_command) {
    pid_t spawnPid = fork();
    if (spawnPid == -1) {
        perror("fork failed");
        return 1;
    }
    else if (spawnPid == 0) { // child process
        // set SIGINT behavior: foreground commands use default, background ignore SIGINT
        if (curr_command->is_bg)
            signal(SIGINT, SIG_IGN);
        else
            signal(SIGINT, SIG_DFL);
        
        // handle input redirection if specified
        if (curr_command->input_file != NULL) {
            int inputFD = open(curr_command->input_file, O_RDONLY);
            if (inputFD == -1) {
                printf("cannot open %s for input\n", curr_command->input_file);
                fflush(stdout);
                exit(1);
            }
            if (dup2(inputFD, 0) == -1) {
                perror("dup2");
                exit(1);
            }
            close(inputFD);
        }
        // handle output redirection if specified
        if (curr_command->output_file != NULL) {
            int outputFD = open(curr_command->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (outputFD == -1) {
                printf("cannot open %s for output\n", curr_command->output_file);
                fflush(stdout);
                exit(1);
            }
            if (dup2(outputFD, 1) == -1) {
                perror("dup2");
                exit(1);
            }
            close(outputFD);
        }
        // for background commands with no redirection, redirect stdin/stdout to /dev/null
        if (curr_command->is_bg) {
            if (curr_command->input_file == NULL) {
                int devNull = open("/dev/null", O_RDONLY);
                dup2(devNull, 0);
                close(devNull);
            }
            if (curr_command->output_file == NULL) {
                int devNull = open("/dev/null", O_WRONLY);
                dup2(devNull, 1);
                close(devNull);
            }
        }
        // execute the command; if execvp fails, print error and exit
        execvp(curr_command->argv[0], curr_command->argv);
        perror("execvp failed");
        exit(1);
    }
    else { // parent process
        if (curr_command->is_bg) {
			// store background process id and notify user
            bg_pids[bg_pids_count++] = spawnPid;
            printf("background pid is %d\n", spawnPid);
            fflush(stdout);
            return 0;
        } else {
            int childStatus;
            waitpid(spawnPid, &childStatus, 0);
            return childStatus;
        }
    }
}

// check_background_processes: reaps any completed background processes
void check_background_processes() {
    int childStatus;
    pid_t pid = waitpid(-1, &childStatus, WNOHANG);
    while (pid > 0) {
		// remove the pid from our list
        for (int i = 0; i < bg_pids_count; i++) {
            if (bg_pids[i] == pid) {
                bg_pids[i] = 0; 
                break;
            }
        }

        if (WIFEXITED(childStatus)) {
            printf("background pid %d is done: exit value %d\n", pid, WEXITSTATUS(childStatus));
        } else if (WIFSIGNALED(childStatus)) {
            printf("background pid %d is done: terminated by signal %d\n", pid, WTERMSIG(childStatus));
        }
        fflush(stdout);
        
        pid = waitpid(-1, &childStatus, WNOHANG);
    }
}

// kill_all_processes: sends SIGKILL to any remaining background processes
void kill_all_processes() {
    for (int i = 0; i < bg_pids_count; i++) {
        if (bg_pids[i] > 0) {
            kill(bg_pids[i], SIGKILL);
            printf("Killed process %d\n", bg_pids[i]);
            fflush(stdout);
        }
    }
}

int main() {
	struct command_line *curr_command;
    const char *homeDir = getenv("HOME");
    char cwd[200];
    int last_status = 0;

	// set up sigint handling: parent ignores ctrl+c
	SIGINT_action.sa_handler = SIG_IGN;
	sigaction(SIGINT, &SIGINT_action, NULL);

	// set up sigtstp handling to toggle foreground-only mode
	SIGTSTP_action.sa_handler = handle_SIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = SA_RESTART;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	while(true)
	{
		check_background_processes();
		curr_command = parse_input();
		
		// skip blank lines and comments
		if (curr_command->argv[0] == NULL || !strcmp(curr_command->argv[0], "#")) {
			free(curr_command);
			continue;
		}

        else if (!strcmp(*curr_command->argv,"exit")) {
            kill_all_processes();
            return 0;
        }

        else if (!strcmp(*curr_command->argv,"cd")) {
			// change directory: no argument means go to home directory
            if (curr_command->argv[1] == NULL) {
                chdir(homeDir);
            }
            else {
                chdir(curr_command->argv[1]);
            }
        }

        else if (!strcmp(*curr_command->argv,"status")) {
			// print exit status or signal of the last foreground process
            if (WIFEXITED(last_status)) {
                printf("exit value %d\n", WEXITSTATUS(last_status));
				fflush(stdout);
            } else if (WIFSIGNALED(last_status)) {
                printf("exit value %d\n", WTERMSIG(last_status));
				fflush(stdout);
            }
        }

        else {
            last_status = execute_command(curr_command);
        }

	}
	return EXIT_SUCCESS;
}
