#ifndef SFISH_H
#define SFISH_H
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/resource.h>

#endif

int help_builtin();
char* pwd_builtin();
int cd_builtin();
char* check_path(char *cmd);

int handle_pipe(char *cmd);

int better_pipe(char *cmd);

void builtin_alarm(int seconds);