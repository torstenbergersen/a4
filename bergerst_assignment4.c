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


struct command_line {
	char *argv[MAX_ARGS + 1];
	int argc;
	char *input_file;
	char *output_file;
	bool is_bg;
};


struct command_line *parse_input() {
	char input[INPUT_LENGTH];
	struct command_line *curr_command = (struct command_line *) calloc(1, sizeof(struct command_line));

	// Get input
	printf(": ");
	fflush(stdout);
	fgets(input, INPUT_LENGTH, stdin);

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

void kill_all_processes() {

}


int execute_command(struct command_line *curr_command) {

	if (curr_command->input_file != NULL) {
		int input = open(curr_command->input_file, O_RDONLY);
		if (input == -1) {
			printf("cannot open %s for input\n", curr_command->input_file);
			fflush(stdout);
			return 1;
		}
		int result = dup2(input, 0);
		if (result == -1) { 
			return 1;
		}
		return 0;
	}

	if (curr_command->output_file != NULL) {
		int output = open(curr_command->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (output == -1) {
			printf("cannot open %s for output\n", curr_command->output_file);
			fflush(stdout);
			return 1;
		}
		int result = dup2(output, 1);
		if (result == -1) { 
			return 1;
		}
		return 0;
	}

	// if (curr_command->is_bg == true) {

	// }

	else {
		int childStatus;
		pid_t spawnPid = fork();
		switch(spawnPid) {
		case -1:
			return 1;
		case 0:
			execvp(curr_command->argv[0], curr_command->argv);
			perror("exec() failed");
			return 0;
		default:
			spawnPid = waitpid(spawnPid, &childStatus, 0);
			return 0;
		}
		return 0;
	}
}

int main() {
	struct command_line *curr_command;
    const char *homeDir = getenv("HOME");
    char cwd[200];
    int last_status = 0;

	while(true)
	{
		curr_command = parse_input();
		
		if (curr_command->argv[0] == NULL || !strcmp(curr_command->argv[0], "#")) {
			continue;
		}
		

        else if (!strcmp(*curr_command->argv,"exit")) {
            // kill_all_processes();
            return 0;
        }

        else if (!strcmp(*curr_command->argv,"cd")) {
            if (curr_command->argv[1] == NULL) {
                // printf("%s\n", getcwd(cwd, sizeof(cwd)));
                chdir(homeDir);
                // printf("%s\n", getcwd(cwd, sizeof(cwd)));
            }
            else {
                // printf("%s\n", getcwd(cwd, sizeof(cwd)));
                chdir(curr_command->argv[1]);
                // printf("%s\n", getcwd(cwd, sizeof(cwd)));
        
            }
        }

        else if (!strcmp(*curr_command->argv,"status")) {
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
