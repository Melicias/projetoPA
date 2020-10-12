/**
* @file main.c
* @brief Description
* @date 2020-10-6
* @author Francisco Melicias - 2191727 & Diogo Francisco - 2191253
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "debug.h"
#include "memory.h"
#include "args.h"

struct sigaction act_info;
struct tm tm;
struct FileCommand { 
    FILE* fp;
	char* command;
}; 


extern char **environ;

time_t t;
int app_ex;
int stdout_ex;
int stderr_ex;
bool keepGoingWithCommands;

char *inputString(FILE* fp, size_t size);
void executeLinesFromFile(char *sFile);
void makeSignalFile();
void takeCareOfSignalInfo(int signal, siginfo_t *siginfo, void *context);
bool isCommandValid(char* command);
void runShellCommand(char* command);
struct FileCommand checkForChannelRedirected(char* command);
char** removeSpacesAndSplitForArray(char* command);
char *subString(char *string, int position, int length);


int main(int argc, char *argv[]){
	
	t = time(NULL);
	tm = *localtime(&t);
	app_ex = 0;
	stdout_ex = 0;
	stderr_ex = 0;
	keepGoingWithCommands = true;

	struct gengetopt_args_info args;

	// gengetopt parser
	if(cmdline_parser(argc, argv, &args))
		ERROR(1, "[Error]: cmdline_parser execution.\n");

	// all the option only must be used alone, so 1 is always from the name of file
	if(argc > 3)
		ERROR(2, "[Error]: All the option must to be used on their own.\n");

	if(args.file_given){
		executeLinesFromFile(args.file_arg);
		return 0;
	}

	if(args.signalfile_given){
		makeSignalFile();
	}
		
	act_info.sa_sigaction = takeCareOfSignalInfo;
	sigemptyset(&act_info.sa_mask);
	act_info.sa_flags = 0;           	//fidedigno
	act_info.sa_flags |= SA_SIGINFO; 	//info adicional sobre o sinal
	//act_info.sa_flags |= SA_RESTART; 	//recupera chamadas bloqueantes

	if(sigaction(SIGINT, &act_info, NULL) < 0)
		ERROR(4, "sigaction (SIGINT)");
	if(sigaction(SIGUSR1, &act_info, NULL) < 0)
		ERROR(5, "sigaction (SIGUSR1)");
	if(sigaction(SIGUSR2, &act_info, NULL) < 0)
		ERROR(6, "sigaction (SIGUSR2)");
			
	

	do{
		char *input;
		printf ("nanoshell$: ");
		input = inputString(stdin, 256);
		if(isCommandValid(input)){
			if(strstr(input, "bye") == NULL) {
				runShellCommand(input);
			}else{
				keepGoingWithCommands = false;
				printf("[INFO] bye command detected. Terminating nanoShell");
			}
		}else{
			printf("[ERROR] Wrong request '%s'\n",input); // error 7
		}

		free(input);
		
	}while(keepGoingWithCommands);
	

	
	// gengetopt: release resources
	//cmdline_parser_free(&args);

	return 0;
}

/*
	Opens the file
	return FILE
*/
void executeLinesFromFile(char *sFile){
	FILE *file = fopen(sFile, "r");
    if (file == NULL)
        ERROR(3, "[Error]: cannot open file <%s>. -- %s\n",sFile, strerror(errno));
	
	char * line = NULL;
    size_t len = 0;
    ssize_t read;
	int commandNumber = 0;
	printf("[INFO] executing from file '%s'",sFile);
	while ((read = getline(&line, &len, file)) != -1) {
		if(read != 1 && line[0] != '#'){
			if(isCommandValid(line)){
				if(strstr(line, "bye") == NULL) {
					commandNumber += 1;
					printf("[command #%d]: %s\n",commandNumber, line);
					runShellCommand(line);
				}else{
					printf("[INFO] bye command detected. Terminating nanoShell");
					return;
				}
			}else{
				printf("[ERROR] Wrong request '%s'\n",line); // error 7
			}
		}
    }
}

void makeSignalFile(){
	FILE *outputFile = fopen("signal.txt", "wab+");
	fprintf(outputFile, "kill -SIGINT %d\nkill -SIGUSR1 %d\nkill -SIGUSR2 %d",getpid(),getpid(),getpid());
	fclose(outputFile);
	printf("[INFO] created file  '%s'\n", "signal.txt");
}

void takeCareOfSignalInfo(int signal, siginfo_t *siginfo, void *context) {
	(void)context;
	/* Cópia da variável global errno */
	int aux = errno;
	
	keepGoingWithCommands = false;
	switch(signal){
		case 2: //SIGINT 
			printf("\n[INFO]\tPID who send the signal: %ld \n\tUID who send the signal: %ld\n", (long)siginfo->si_pid, (long)siginfo->si_uid);
			break;
		case 10: //SIGUSR1 
			printf("\n[INFO] nanoShell started at: %d-%02d-%02dT%02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			break;
		case 12: ;//SIGUSR2
			char fileName[50];
			sprintf(fileName,"nano_shell_status_%d.%02d.%02d_%02dh%02d.%02d.txt", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			FILE *outputFile = fopen(fileName, "wab+");
			fprintf(outputFile, "%d executions of applications\n%d executions with STDOUT redir \n%d execution with STDERR redir",app_ex,stdout_ex,stderr_ex);
			fclose(outputFile);
			printf("\n[INFO] created file  '%s'\n", fileName);
			break;
		default:
			printf("\n[INFO] it wasn't redirected to any of the 3 signals");

	}

	/* Restaura valor da variável global errno */
	errno = aux;
}

//https://stackoverflow.com/questions/16870485/how-can-i-read-an-input-string-of-unknown-length
char *inputString(FILE* fp, size_t size){
	//The size is extended by the input with the value of the provisional
    char *str;
    int ch;
    size_t len = 0;
    str = realloc(NULL, sizeof(char)*size);//size is start size
    if(!str)return str;
    while(EOF!=(ch=fgetc(fp)) && ch != '\n'){
        str[len++]=ch;
        if(len==size){
            str = realloc(str, sizeof(char)*(size+=16));
            if(!str)return str;
        }
    }
    str[len++]='\0';

    return realloc(str, sizeof(char)*len);
}


bool isCommandValid(char* command){
	for(int i = 0; i<(int)strlen(command); ++i)
		if (command[i] == '\'' || command[i] == '"' || command[i] == '|' || command[i] == '*' || command[i] == '?')
			return false;
	return true;
}


void runShellCommand(char* command){
	pid_t pid = fork();
	if (pid == 0) {			// Processo filho 
		struct FileCommand fc = checkForChannelRedirected(command);

		char ** commandArray;
		if(fc.fp == NULL){
			commandArray = removeSpacesAndSplitForArray(command);
		}else{
			commandArray = removeSpacesAndSplitForArray(fc.command);
		}
		
		execvp(commandArray[0],commandArray);
		if(fc.fp != NULL)
			fclose(fc.fp);

		exit(0);
	} else if (pid > 0) {	// Processo pai 
		wait(NULL);
	} else					// < 0 - erro
		ERROR(8, "[ERRO] problem with fork()");
}

struct FileCommand checkForChannelRedirected(char* command){
	struct FileCommand fc;
	fc.command = NULL; fc.fp = NULL;
	char* substring = strstr(command, " > ");
	if(substring != NULL) {
		fc.command = subString(command, 1, (substring - command));
		char* fileName = subString(command, (substring - command)+3, strlen(command));
		if(fileName != NULL)
			fc.fp = freopen(fileName, "w", stdout);
		return fc;
	}
	substring = strstr(command, " >> ");
	if(substring != NULL) {
		fc.command = subString(command, 1, (substring - command));
		char* fileName = subString(command, (substring - command)+4, strlen(command));
		if(fileName != NULL)
			fc.fp = freopen(fileName, "a", stdout);
		return fc;
	}
	substring = strstr(command, " 2> ");
	if(substring != NULL) {
		fc.command = subString(command, 1, (substring - command));
		char* fileName = subString(command, (substring - command)+4, strlen(command));
		if(fileName != NULL)
			fc.fp = freopen(fileName, "w", stderr);
		return fc;
	}
	substring = strstr(command, " 2>> ");
	if(substring != NULL) {
		fc.command = subString(command, 1, (substring - command));
		char* fileName = subString(command, (substring - command)+5, strlen(command));
		if(fileName != NULL)
			fc.fp = freopen(fileName, "a", stderr);
		return fc;
	}
	return fc;
}

char** removeSpacesAndSplitForArray(char* command){
	char ** ret = NULL;
	char * p = strtok (command, " ");
	int n_spaces = 0;

	// split string and append tokens to 'rerturn'
	while (p) {
		ret = realloc (ret, sizeof (char*) * ++n_spaces);

		if (ret == NULL)
			exit (-1); // memory allocation failed - disparar erro

		ret[n_spaces-1] = p;

		p = strtok (NULL, " ");
	}

	/* realloc one extra element for the last NULL */
	ret = realloc (ret, sizeof (char*) * (n_spaces+1));
	ret[n_spaces] = 0;

	/*for (int i = 0; i < sizeof(commandArray); ++i)
  			printf ("res[%d] = %s\n", i, commandArray[i]);*/

	return ret;
}

//https://www.programmingsimplified.com/c/source-code/c-substring
char *subString(char *string, int position, int length){
   char *p;
   int c;
 
   p = malloc(length+1);
   if (p == NULL){
	   // dar fix neste erro
      printf("Unable to allocate memory.\n");
      return NULL;
   }
 
   for (c = 0; c < length; c++){
      *(p+c) = *(string+position-1);      
      string++;  
   }
 
   *(p+c) = '\0';
 
   return p;
}
