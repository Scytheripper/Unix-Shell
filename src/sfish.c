#include "sfish.h"

volatile sig_atomic_t alarm_sig = 0;
char *PAST;

int help_builtin(){
	printf("%s\n", "help: Print this menu");
	printf("%s\n", "exit: Exit the shell");
	printf("%s\n", "cd[-],[PATH]   Change directory to specified path");
	printf("%s\n", "pwd:  Print current absolute path");
	return 0;
}

char* pwd_builtin(){
	int buff_size = 1024;
	char *buff = (char *)malloc(buff_size);
	while(getcwd(buff, buff_size) == NULL){
		buff_size = buff_size + 1;
	}
	return buff;
}

int cd_builtin(char *cd){
	int check;
	// set to HOME
	if(cd == 0){
		check = chdir(getenv("HOME"));
		if(check < 0){
			fprintf(stderr, "%s\n", "ERROR: INVALID DESTINATION");
		}
		else{
			char *new_pwd = pwd_builtin();
			setenv("PWD", new_pwd, 1);
			free(new_pwd);
		}
	}
	// try the user's input
	else{
		if(strcmp(cd, "-") == 0){
			char *kek = pwd_builtin();
			chdir(getenv("LOL"));
			setenv("LOL", kek, 1);
			free(kek);
			char *new_pwd = pwd_builtin();
			setenv("PWD", new_pwd, 1);
			free(new_pwd);
		}
		else{
			char *kek = pwd_builtin();
			setenv("LOL", kek, 1);
			free(kek);
			if(chdir(cd) < 0){
				fprintf(stderr, "%s\n", "ERROR: INVALID DESTINATION");
			}
			else{
				char *new_pwd = pwd_builtin();
				setenv("PWD", new_pwd, 1);
				free(new_pwd);
			}
		}
	}
	return 0;
}


/* ALLOCATED MEMORY MUST BE FREED*/
// returns the path to an executable program
// or NULL if cmd does not exist
char* check_path(char *cmd){
	struct stat freestat;
	//check if the prog is good on its own.
	char *dot = ".";
	char *semi = ":";
	char *slash = "/";
	char *buffer = (char *)malloc(strlen(dot) +strlen(slash) + strlen(cmd)  + 1);
	strcpy(buffer, dot);
	strcat(buffer, slash);
	strcat(buffer, cmd);
	if(stat(buffer, &freestat) == 0){
		return buffer;
	}
	else if(stat(cmd, &freestat) == 0){
		free(buffer);
		char *buffer = (char *)malloc(strlen(cmd)  + 1);
		strcpy(buffer, cmd);
		return buffer;
	}
	//tokenize the path and check it
	else{
		free(buffer);// free the buffer
		char *paths = malloc(strlen(getenv("PATH")) + 1);
		strcpy(paths, getenv("PATH"));
		char *token = strtok(paths, semi);
		int size;
		while(token != NULL){
			size = 1 + strlen(cmd) + strlen(slash) + strlen(token);
			buffer = (char *)malloc(size);
			strcpy(buffer, token);
			strcat(buffer, slash);
			strcat(buffer, cmd);
			if(stat(buffer, &freestat) == 0){
				free(paths);
				return buffer;
			}
			free(buffer);
			token = strtok(NULL,semi);
		}
		free(paths);
	}
	return NULL;
}

int parse_exec(char * cmd){
	int count = 0;
	int args = 2; // cmd and NULL
	char *save_ptr;
	char *space = " ";
	while(*(cmd + count) != 0){
		if (*(cmd + count) == ' ') args++;
		count++;
	}
	char *arg[args];
	char * token = strtok_r(cmd, space, &save_ptr);
	char *cmd_path = check_path(token);
	int new_args = 0;
	if(cmd_path == NULL){
		fprintf(stderr, "%s\n", "Program not found");
		return -1;
	}
	arg[new_args] = cmd_path;
	new_args++;
	token = strtok_r(NULL, space, &save_ptr);
	while(token != NULL){
		arg[new_args] = token;
		new_args++;
		token = strtok_r(NULL, space, &save_ptr);
	}
	arg[new_args] = NULL;
	printf("%s\n", cmd_path);
	pid_t pid = fork();
	if(pid == 0){
		execv(cmd_path, arg);
	}
	else if (pid < 0){
		fprintf(stderr, "%s\n", "Fork Error");
		return -1;
	}
	return 0;
}

int handle_pipe(char *cmd){
	//parse the command for the args and exec,
	//any errors, return -1;
	//first command is good no need to check
	char *s = " ";

	int pc[2];
	//get programs to pipe 2 at time
	char *cmd1 = NULL;
	char *cmd2 = NULL;
	char args1[1024];
	char args2[1024];
	char *save_ptr;
	// HOW MANY TIMES TO PIPE
	int off = 0;
	int pipes = 0;
	while(*(cmd + off) != 0){
		if(*(cmd + off) == '|') pipes ++;
		off++;
	}
	char *token = strtok_r(cmd,s, &save_ptr);
	while(pipes && token != NULL){
		cmd1 = check_path(token);// REMEMBER TO FREE
		if(cmd1 == NULL){
			fprintf(stderr, "%s : cannot be found\n", token);
			return -1;
		}
		strcpy(args1,cmd1);
		token = strtok_r(NULL, s, &save_ptr);
		while(strcmp(token, "|") != 0 && token != NULL){
			strcat(args1, ",");
			strcat(args1,token);
			token = strtok_r(NULL, s, &save_ptr);
		}
		//MOVE ON TO PROCESS 2
		token = strtok_r(NULL, s, &save_ptr);
		cmd2 = check_path(token);
		if(cmd2 == NULL){
			free(cmd1);
			fprintf(stderr, "%s : cannot be found\n", token);
			return -1;
		}
		strcpy(args2,cmd2);
		token = strtok_r(NULL, s, &save_ptr);
		if(pipes == 1){
			while(token != NULL){
				strcat(args2, ",");
				strcat(args2,token);
				token = strtok_r(NULL, s, &save_ptr);

			}
		}
		else{
			while(strcmp(token, "|") != 0){
				strcat(args2, ",");
				strcat(args2,token);
				token = strtok_r(NULL, s, &save_ptr);
			}
		}
		pipe(pc);
		pid_t kid =fork();
		if(kid == 0){
			dup2(pc[1], 1);// THE WRITE END
			close(pc[0]);
			execl(cmd1, args1, NULL);
		}
		else if(kid < 0){
			fprintf(stderr, "%s\n", "FORK ERROR");
		}

		//wait(NULL);
		pid_t pid =fork();
		if(pid == 0){
			// THE RIGHT END
			dup2(pc[0], 0);
			if(pipes == 1) close(pc[1]);
			execl(cmd2, args2, NULL);
		}
		else if(pid < 0){
			fprintf(stderr, "%s\n", "FORK ERROR");
		}

		wait(NULL);
		if(pipes == 1){
			close(pc[0]);
			close(pc[1]);
			free(cmd1);
			free(cmd2);
		}
		pipes--;
		token = strtok_r(NULL,s, &save_ptr);
	}
	return 0;
}

int args_counter(char *cmd){
	int counter = 0;
	int args = 2;
	while(*(cmd + counter) != 0){
		if(*(cmd + counter) == ' ') args++;
		counter++;
	}
	return args;
}

void onAlarm(int s) {
	s = alarm_sig;
	printf("%d seconds are up!\n", s);
}

void builtin_alarm(int seconds){
	alarm_sig = seconds;
	signal(SIGALRM, onAlarm);
	sleep(seconds);
	printf("%d seconds are up!\n", seconds);
}

int better_pipe(char *cmd){
	//tokenize by |
	char *pipec = "|";
	int kek;
	int status;
	char *save_ptr;
	char *save_ptr2;
	char *token = strtok_r(cmd, pipec, &save_ptr);
	int pc[2];
	char *cmd1;
	pipe(pc);
	while(token != NULL){
		//get the amount of args
		char *cmd1_args[args_counter(token)];
		int curr_arg = 0;
		//parse the token by space
		char* cmd_token = strtok_r(token, " ", &save_ptr2);
		cmd1 = check_path(cmd_token);
		cmd1_args[curr_arg] = cmd1;
		curr_arg ++;
		if(cmd1 == NULL) break;
		cmd_token = strtok_r(NULL, " ", &save_ptr2);
		while(cmd_token != NULL){
			cmd1_args[curr_arg] = cmd_token;
			cmd_token = strtok_r(NULL, " ", &save_ptr2);
			curr_arg++;
		}
		cmd1_args[curr_arg] = NULL;

		token = strtok_r(NULL, pipec, &save_ptr);
		char *cmd2_args[args_counter(token)];
		curr_arg = 0;

		cmd_token = strtok_r(token, " ", &save_ptr2);
		char * cmd2 = check_path(cmd_token);
		cmd2_args[curr_arg] = cmd2;
		curr_arg ++;
		if(cmd2 == NULL){
			fprintf(stderr, "%s\n", "Program not found");
			break;
		}
		cmd_token = strtok_r(NULL, " ", &save_ptr2);
		while(cmd_token != NULL){
			cmd2_args[curr_arg] = cmd_token;
			cmd_token = strtok_r(NULL, " ", &save_ptr2);
			curr_arg++;
		}
		cmd2_args[curr_arg] = NULL;

		pipe(pc);
		pid_t pid = fork();
		if(pid == 0){
			dup2(pc[1], 1);
			close(pc[0]);
			execv(cmd1,cmd1_args);
		}
		else if(pid < 0){
			perror("Fork");
		}

		pid_t pid2 = fork();
		if(pid2 == 0){
			dup2(pc[0], 0);
			close(pc[1]);
			execv(cmd2,cmd2_args);
		}
		else if(pid2 < 0){
			perror("Fork");
		}
		close(pc[0]);
		close(pc[1]);
		//pick up children
		while((kek = wait(&status)) != -1){

		}
		free(cmd1);
		free(cmd2);
		break;
		token = strtok_r(NULL,"|",&save_ptr );
	}
	return 0;
}
