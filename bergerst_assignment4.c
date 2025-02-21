#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define INPUT_LENGTH 2048
#define MAX_ARGS		 512


struct command_line
{
	char *argv[MAX_ARGS + 1];
	int argc;
	char *input_file;
	char *output_file;
	bool is_bg;
};


struct command_line *parse_input()
{
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

int main()
{
	struct command_line *curr_command;
    const char *homeDir = getenv("HOME");
    char cwd[200];
	while(true)
	{
		curr_command = parse_input();
        if (!strcmp(*curr_command->argv,"exit")) {
            // kill_all_processes();
            return 0;
        }
        // printf("%s\n",curr_command->argv[1]);

        else if (!strcmp(*curr_command->argv,"cd")) {
            if (curr_command->argv[1] == NULL) {
                printf("%s\n", getcwd(cwd, sizeof(cwd)));
                chdir(homeDir);
                printf("%s\n", getcwd(cwd, sizeof(cwd)));
            }
            else {
                printf("%s\n", getcwd(cwd, sizeof(cwd)));
                chdir(curr_command->argv[1]);
                printf("%s\n", getcwd(cwd, sizeof(cwd)));
        
            }
        }
	}
	return EXIT_SUCCESS;
}
