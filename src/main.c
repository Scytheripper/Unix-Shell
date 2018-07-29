#include "sfish.h"
#include "debug.h"

void child_dead();
volatile sig_atomic_t the_pid = 0;
/*
 * As in previous hws the main function must be in its own file!
 */
char *PRE_CMD =  "<sshivraj><";
char *END_CMD = "> $";
const char* sp = " ";
/*Save the standard input and output fild descriptors*/
int STANDARD_IN;
int STANDARD_OUT;
int STANDARD_ERR;
struct stat freestat;
char *prog;
int dont_wait;
int main(int argc, char const *argv[], char* envp[]){
    /* DO NOT MODIFY THIS. If you do you will get a ZERO. */
    rl_catch_signals = 0;
    /* This is disable readline's default signal handlers, since you are going to install your own.*/
    // initial cwd
    char *cwd = pwd_builtin();
    int read_line_buff = strlen(PRE_CMD) + strlen(cwd) + strlen(END_CMD) + 1;
    char *read_line_prompt = (char*)malloc(read_line_buff);//malloc the space for the prompt
    //clear the buffer
    int buff_clear = 0;
    for(buff_clear = 0; buff_clear < read_line_buff; buff_clear++){
        *(read_line_prompt + buff_clear) = 0;
    }
    strcpy(read_line_prompt, PRE_CMD);
    strcat(read_line_prompt,cwd);
    strcat(read_line_prompt, END_CMD);
    pid_t pid;
    char *cmd;
    char *curr_arg;
    int builtin_error = 0;
    while((cmd = readline(read_line_prompt)) != NULL) {
        dont_wait = 0;
        builtin_error = 0;
        int builtin_redirect = 0;
        //make a copy of the cmd string
        char *cmd_copy = malloc(strlen(cmd) + 1);
        strcpy(cmd_copy,cmd);
        //tokenize the cmd string
        if(*cmd != 0){
            int off_set = 0;
            while(*(cmd + off_set) != 0){
                if(*(cmd + off_set) == ' '){
                    builtin_error = 1;
                }
                if(*(cmd + off_set) == '>'){
                    builtin_redirect = 1;
                }
                off_set++;
            }
            curr_arg = strtok(cmd, sp);
        }

        else{
            curr_arg = cmd;
        }
        if (strcmp(curr_arg, "exit")==0){
            if(builtin_error == 1){
               fprintf(stderr, "%s\n", "ERROR: This builtin needs no args");
            }
            else
            break;
        }

        else if(strcmp(curr_arg, "help") == 0){
            if(builtin_error == 1 && builtin_redirect == 0){
                fprintf(stderr, "%s\n", "ERROR: This builtin needs no args");
            }
            else if(builtin_redirect == 1){
                // get the filename to redirect to.
                strtok(NULL,sp);
                curr_arg = strtok(NULL,sp);
                if(curr_arg == NULL){
                    fprintf(stderr, "%s\n", "NO FILE SPECIFIED");
                }
                FILE *refile = fopen(curr_arg, "w");
                if(refile == NULL){
                    fprintf(stderr, "%s\n", "INVALID FILE");
                }
                pid = fork();
                the_pid = getpid();
                if(pid == 0){
                    dup2(fileno(refile), STDOUT_FILENO);
                    help_builtin();
                    _exit(0);
                }
                else if(pid < 0){
                    fprintf(stderr, "%s\n", "FORK ERROR");
                }
                else{
                    wait(NULL);
                    fclose(refile);
                }
            }
            else{
                pid = fork();
                the_pid = getpid();
                if(pid == 0){
                    help_builtin();
                    _exit(0);
                }
            }
        }
        else if(strcmp(curr_arg, "alarm") == 0){
            char *arg = strtok(NULL, sp);
            int sec = 0;
            if(arg == NULL){
                fprintf(stderr, "%s\n", "Invalid number");
            }
            else{
                sec = atoi(arg);
                if(sec == 0){
                    fprintf(stderr, "%s\n", "Invalid number");
                }
                else{
                    pid = fork();
                    the_pid = getpid();
                    if(pid == 0){
                        builtin_alarm(sec);
                        _exit(0);
                    }
                }
            }
        }
        else if(strcmp(curr_arg, "pwd") == 0){
            if(builtin_error == 1 && builtin_redirect == 0){
                fprintf(stderr, "%s\n", "ERROR: This builtin needs no args");
            }
            else if(builtin_redirect == 1){
                // get the filename to redirect to.
                strtok(NULL,sp);
                curr_arg = strtok(NULL,sp);
                if(curr_arg == NULL){
                    fprintf(stderr, "%s\n", "NO FILE SPECIFIED");
                }
                FILE *refile = fopen(curr_arg, "w");
                if(refile == NULL){
                    fprintf(stderr, "%s\n", "INVALID FILE");
                }
                pid = fork();
                the_pid = getpid();
                if(pid == 0){
                    dup2(fileno(refile), STDOUT_FILENO);
                    char *printp = pwd_builtin();
                    printf("%s\n", printp);
                    free(printp); //free the string
                    _exit(0);
                }
                else if(pid < 0){
                    fprintf(stderr, "%s\n", "FORK ERROR");
                }
                else{
                    wait(NULL);
                    fclose(refile);
                }
            }
            else{
                pid = fork();
                the_pid = getpid();
                if(pid == 0){
                    char *printp = pwd_builtin();
                    printf("%s\n", printp);
                    free(printp); //free the string
                    _exit(0);
                }
                else if(pid < 0) fprintf(stderr, "%s\n", "FORK ERROR");
            }
        }
        //DON'T NEED TO FORK THIS
        else if(strcmp(curr_arg, "cd") == 0){
            //curr_arg = strtok(NULL,sp);
            cd_builtin(cmd +3);
        }
        // after this we will use cmd
        //curr_arg = cmd;
        // check to see if the cmd is valid on it's own
        else{
                int off_set = 0;
                int pipe_sig = 0;
                while(*(cmd_copy + off_set) != 0){
                    if(*(cmd_copy + off_set) == '|') pipe_sig = 1;
                    off_set++;
                }
                if(pipe_sig){
                    better_pipe(cmd_copy);
                }
            // check to see if the cmd is valid on it's own
            prog = check_path(curr_arg);//USES ALLOCATED MEMORY, contains JUST THE PROGRAM NAME.
            if(prog != NULL){
                //get the args for the program, delim by spaces.
                //count spaces to find the size of the args array
                int arg_size = 0;
                int off_set = 0;
                while(*(cmd_copy + off_set) != 0){
                    if(*(cmd_copy + off_set) == ' ') arg_size++;
                    off_set++;
                }
                if(arg_size  > 0){
                    char *args[arg_size + 1 + 1];
                    char *some_arg = strtok(cmd_copy,sp);
                    int index = 0;
                    int input_sig = 0;// THIS WILL CHANCGE IS > OR < OR |
                    int output_sig = 0;
                    char *outstr = NULL;
                    char *instr = NULL;
                    //create the args array
                    while((some_arg != NULL)){
                        if(strcmp(some_arg,">") == 0){
                            //get the file name
                            outstr = strtok(NULL, sp);
                            output_sig = 1;
                            some_arg = strtok(NULL, sp);
                        }
                        else if(strcmp(some_arg,"<") == 0){
                            input_sig = 2;
                            instr = strtok(NULL, sp);
                            some_arg = strtok(NULL, sp);
                        }
                        else{
                            if((input_sig == 0) && (output_sig == 0)){
                                args[index] = some_arg;
                                some_arg = strtok(NULL,sp);
                                index++;
                            }
                        }
                    }
                    int kpid = fork();
                    the_pid = getpid();
                    if(kpid ==0){
                        FILE *outFile = NULL;
                        FILE *inFile = NULL;
                        // if > or < get the filename, open the file and change the appropriate file descriptor.
                        int redirect_error = 0;
                        if(output_sig){
                            outFile = fopen(outstr, "w");
                            if(outFile != NULL){
                                STANDARD_OUT = dup(fileno(stdout));
                                dup2(fileno(outFile), STDOUT_FILENO);
                            }
                            else{
                                redirect_error = -1;
                            }
                        }
                        if(input_sig){
                            if(instr != NULL){
                                inFile = fopen(instr, "r");
                                if(inFile != NULL){
                                    STANDARD_IN = dup(fileno(stdin));
                                    dup2(fileno(inFile), STDIN_FILENO);
                                }
                                else redirect_error = -1;
                            }
                            else{
                                redirect_error = -1;
                            }
                        }

                        if(redirect_error == 0){
                            // RUN WITH ARGS
                            if(index > 0){
                                args[index] = NULL;
                                //int kpid = fork();
                                if(kpid == 0) {
                                    execv(prog,args);//EXECV the program.
                                    fprintf(stderr, "%s\n", "EXECV ERROR");
                                }
                                else if(kpid < 0) fprintf(stderr, "%s\n", "FORK ERORR");
                                /*else{
                                    wait(NULL);
                                    dont_wait = 1;
                                    if(outFile != NULL)fclose(outFile);
                                    if(inFile != NULL){
                                        fclose(inFile);
                                    }
                                }*/
                            }
                            //NO OTHER ARGS TO WORRY ABOUT
                            else{
                                pid = fork();
                                the_pid = getpid();
                                if(pid == 0) execv(prog,(char *[]){" ", NULL});//EXECV the program.
                                else if(pid < 0) fprintf(stderr, "%s\n", "FORK ERORR");
                            }
                        }
                        else{
                            fprintf(stderr, "%s\n", "Invalid Redirect-File");
                        }
                    }
                }

                /* EXECUTE THE PROGROD WITH NO ARGS*/
                else{
                    pid = fork();
                    the_pid = getpid();
                    if(pid == 0) execv(prog,(char *[]){" ", NULL});//EXECV the program.
                    else if(pid < 0) fprintf(stderr, "%s\n", "FORK ERORR");
                }
            }
            else{
                fprintf(stderr, "%s\n", "Program not found");
            }
        }
        //else invalid command
        //fprintf(stderr, "%s\n", "INVALID OPERATION");
        wait(NULL);
        if(prog != NULL){
            free(prog);
            prog = NULL;
        }
        //reset the filedescriptors
        dup2(STANDARD_OUT, STDOUT_FILENO);
        dup2(STANDARD_IN, STDIN_FILENO);
        signal(SIGCHLD, child_dead);
        /* All your debug print statements should use the macros found in debu.h */
        /* Use the `make debug` target in the makefile to run with these enabled. */
        //info("Length of command entered: %ld\n", strlen(cmd));
        /* You WILL lose points if your shell prints out garbage values. */
        //free the cwd then reset;
        free(cwd);
        cwd = pwd_builtin();
        free(read_line_prompt);
        free(cmd);
        free(cmd_copy);
        read_line_buff = strlen(PRE_CMD) + strlen(cwd) + strlen(END_CMD) + 1;
        read_line_prompt = (char *)malloc(read_line_buff);
        buff_clear = 0;
        for(buff_clear = 0;buff_clear < read_line_buff; buff_clear++){
            *(read_line_prompt + buff_clear) = 0;
        }
        strcpy(read_line_prompt, PRE_CMD);
        strcat(read_line_prompt,cwd);
        strcat(read_line_prompt, END_CMD);
    }

    /* Don't forget to free allocated memory, and close file descriptors. */
    free(cmd);
    free(cwd);
    free(read_line_prompt);
    return EXIT_SUCCESS;
}

void child_dead(){
    printf("Child with PID:%d has died. It spent X milliseconds utilizing the CPU\n", the_pid);
}
