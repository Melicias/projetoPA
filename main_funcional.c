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
    FILE* fpStdout;
	FILE* fpStderr;
	char* command;
}; 

extern char **environ;

time_t t;
int app_ex = 0;
int stdout_ex = 0;
int stderr_ex = 0;
bool keepGoingWithCommands = true;
int maxCommand = -1;

char *string[] = {" > ", " >> ", " 2> ", "2>>"};

char *inputString(FILE* fp, size_t size);
void executeLinesFromFile(char *sFile);
void makeSignalFile();
void takeCareOfSignalInfo(int signal, siginfo_t *siginfo, void *context);
bool isCommandValid(char* command);
void runShellCommand(char* command);
struct FileCommand checkForChannelRedirected(char* command);
struct FileCommand openFile(struct FileCommand fc, char* substring, char* command, int removeChar,char* type, char* mode, FILE *stream);
char** removeSpacesAndSplitForArray(char* command);
char *subString(char *string, int position, int length);


int main(int argc, char *argv[]){
	
	t = time(NULL);
	tm = *localtime(&t);

	struct gengetopt_args_info args;

	// gengetopt parser
	if(cmdline_parser(argc, argv, &args))
		ERROR(1, "[Error] cmdline_parser execution.\n");

	// all the option only must be used alone, so 1 is always from the name of file
	if(argc > 3)
		ERROR(2, "[Error] All the option must to be used on their own.\n");

	// if option -f given run all the file and end the program
	if(args.file_given){
		//fazer testes de file
		executeLinesFromFile(args.file_arg);
		return 0;
	}

	// if option -s given, make the signal file
	if(args.signalfile_given){
		makeSignalFile();
	}

	// if option -m given, change the value of maxCommand
	if(args.max_given){
		if(args.max_arg < 1){
			ERROR(9, "[ERROR] invalid value ‘%d’ for -m/--max\n",args.max_arg);
			return 0;
		}
		printf("[INFO] terminates after %d commands\n", args.max_arg);
		maxCommand = args.max_arg;
	}

	// gengetopt: release resources
	cmdline_parser_free(&args);
		
	// signal stuff
	act_info.sa_sigaction = takeCareOfSignalInfo;
	sigemptyset(&act_info.sa_mask);
	act_info.sa_flags = 0;           	//fidedigno
	act_info.sa_flags |= SA_SIGINFO; 	//adicional info about eh signal
	//act_info.sa_flags |= SA_RESTART; 	//recupera chamadas bloqueantes

	if(sigaction(SIGINT, &act_info, NULL) < 0)
		ERROR(4, "sigaction (SIGINT)");
	if(sigaction(SIGUSR1, &act_info, NULL) < 0)
		ERROR(5, "sigaction (SIGUSR1)");
	if(sigaction(SIGUSR2, &act_info, NULL) < 0)
		ERROR(6, "sigaction (SIGUSR2)");
			
	
	// reads from shell and keeps in loop until someone type "bye" or 
	do{
		printf ("nanoshell$: ");
		//waits for an input
		char *input = inputString(stdin, 256);
		//check if the command is valid
		if(isCommandValid(input)){
			//strstr(input, "bye") == NULL -> checks in all the line
			//check if the word "bye" is alone
			if(strcmp(input, "bye") != 0) {
				// try to run the command from the shell
				runShellCommand(input);
				maxCommand--;
			}else{
				keepGoingWithCommands = false;
				printf("[INFO] bye command detected. Terminating nanoShell\n\n");
			}
		}else{
			printf("[ERROR] Wrong request '%s'\n",input); // error 7
		}

		free(input);
		
		if(maxCommand == 0){
			printf("\n[END] Executed %d commands (-m %d)\n", args.max_arg,args.max_arg);
			keepGoingWithCommands = false;
		}

	}while(keepGoingWithCommands);



	return 0;
}

/*
	Opens the file and runs all the lines
*/
void executeLinesFromFile(char *sFile){
	FILE *file = fopen(sFile, "r");
    if (file == NULL)
        ERROR(3, "[Error] cannot open file <%s>. -- %s\n",sFile, strerror(errno));
	
	char * line = NULL;
    size_t len = 0;
    ssize_t read;
	int commandNumber = 0;
	printf("[INFO] executing from file '%s'",sFile);
	// runs all the lines from the file
	while ((read = getline(&line, &len, file)) != -1) {
		if(read != 1 && line[0] != '#'){
			// checks if the command is valid
			if(isCommandValid(line)){
				if(strstr(line, "bye") == NULL) { //check if the line contains "bye", if so, end
					commandNumber += 1;
					printf("[command #%d]: %s\n",commandNumber, line);
					runShellCommand(line);
				}else{
					printf("[INFO] bye command detected. Terminating nanoShell");
					fclose(file);
					return;
				}
			}else{
				printf("[ERROR] Wrong request '%s'\n",line); // error 7
			}
		}
    }
	fclose(file);
}

/*
	creates the signal file with the signals 
*/
void makeSignalFile(){
	FILE *outputFile = fopen("signal.txt", "wab+");
	if(outputFile == NULL){
		printf("[ERROR] creating file 'signal.txt'\n");
		return;
	}
	fprintf(outputFile, "kill -SIGINT %d\nkill -SIGUSR1 %d\nkill -SIGUSR2 %d",getpid(),getpid(),getpid());
	fclose(outputFile);
	printf("[INFO] created file 'signal.txt'\n");
}

/*
	when signal is sent, it will take care of it
*/
void takeCareOfSignalInfo(int signal, siginfo_t *siginfo, void *context) {
	(void)context;
	// copy of errno
	int aux = errno;
	
	keepGoingWithCommands = false;
	switch(signal){
		case 2: //SIGINT 
			printf("\n[INFO]\tPID who send the signal: %ld \n\tUID who send the signal: %ld\n", (long)siginfo->si_pid, (long)siginfo->si_uid);
			exit(0);
			break;
		case 10: //SIGUSR1 
			printf("\n[INFO] nanoShell started at: %d-%02d-%02dT%02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			exit(0);
			break;
		case 12: ;//SIGUSR2
			char fileName[50];
			sprintf(fileName,"nano_shell_status_%d.%02d.%02d_%02dh%02d.%02d.txt", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			FILE *outputFile = fopen(fileName, "wab+");
			fprintf(outputFile, "%d executions of applications\n%d executions with STDOUT redir \n%d execution with STDERR redir",app_ex,stdout_ex,stderr_ex);
			fclose(outputFile);
			printf("\n[INFO] created file  '%s'\n", fileName);
			exit(0);
			break;
		default:
			printf("\n[INFO] it wasn't redirected to any of the 3 signals");
			exit(0);
	}

	/* restores the value of errno */
	errno = aux;
}

/*
	gets the input string

	source:
		https://stackoverflow.com/questions/16870485/how-can-i-read-an-input-string-of-unknown-length
*/
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

/*
Check if the string is valid (if the string has any of those chars, it returns false)
Of course we could use strchr(str, '!') != NULL; for example, but its not worth doing it

*/
bool isCommandValid(char* command){
	for(int i = 0; i<(int)strlen(command); ++i)
		if (command[i] == '\'' || command[i] == '"' || command[i] == '|' || command[i] == '*' || command[i] == '?')
			return false;
	return true;
}

/*
	runs the command in the shell with execvp
*/
void runShellCommand(char* command){
	pid_t pid = fork();
	if (pid == 0) {			// son
		// check if the command needs to redirect
		struct FileCommand fc = checkForChannelRedirected(command);

		// roganizes the information into an array
		char ** commandArray;
		if(fc.fpStdout == NULL && fc.fpStderr == NULL){
			commandArray = removeSpacesAndSplitForArray(command);
		}else{
			commandArray = removeSpacesAndSplitForArray(fc.command);
		}

		// executes the command
		execvp(commandArray[0],commandArray);
		app_ex += 1;

		if(fc.fpStdout != NULL){
			fclose(fc.fpStdout);
		}
		if(fc.fpStderr != NULL){
			fclose(fc.fpStderr);
		}

		exit(0);
	} else if (pid > 0) {	// dad
		wait(NULL); // wait for son
	} else					// < 0 - erro
		ERROR(8, "[ERROR] problem with fork()");
}

//falta fazer a verificacao de "comando > out.txt 2> err.txt"
// uname -a 2> err.txt > out.txt 
/*
	get the command and check if there is any redirectement
	if so, it will open the file for it
*/

struct FileCommand checkForChannelRedirected(char* command){
	//uname > /home/user/Desktop/PA/projetoPA/t.txt
	//uname -y 2>> /home/user/Desktop/PA/projetoPA/e.txt
	struct FileCommand fc;
	fc.fpStdout = NULL;
	fc.fpStderr = NULL;
	fc.command = NULL;
	char* substring = strstr(command, " > ");
	if(substring != NULL) {
		fc = openFile(fc,substring,command,4,"stdout","w", stdout);	
	}
	substring = strstr(command, " >> ");
	if(substring != NULL) {
		fc = openFile(fc,substring,command,5,"stdout","a",stdout);
	}
	substring = strstr(command, " 2> ");
	if(substring != NULL) {
		fc = openFile(fc,substring,command,5,"stderr","w",stderr);
	}
	substring = strstr(command, " 2>> ");
	if(substring != NULL) {
		fc = openFile(fc,substring,command,6,"stderr","a",stderr);
	}
	return fc;
}

/*

*/
struct FileCommand openFile(struct FileCommand fc, char* substring, char* command, int removeChar,char* type, char* mode, FILE* stream){
	if(fc.command == NULL){
		fc.command = subString(command, 1, (substring - command));
		char* subsubstring = strstr(fc.command, " > ");
		if(subsubstring != NULL) {
			printf("\n\n%s\n\n",fc.command);
		}
		subsubstring = strstr(fc.command, " >> ");
		if(subsubstring != NULL) {
			printf("\n\n%s\n\n",fc.command);
		}
		subsubstring = strstr(fc.command, " 2> ");
		if(subsubstring != NULL) {
			*subsubstring = '\0';
			printf("\n\n%s\n\n",fc.command);
		}
		subsubstring = strstr(fc.command, " 2>> ");
		if(subsubstring != NULL) {
			printf("\n\n%s\n\n",fc.command);
		}
	}
	char* fileName = subString(command, (substring - command)+removeChar, strlen(command));
	
	char* removeC = strchr(fileName, ' ');
	if (removeC != NULL)
		*removeC = '\0';

	if(fileName != NULL){
		printf("[INFO] %s redirected to '%s'\n", type, fileName);
		if(stream == stdout){
			fc.fpStdout = freopen(fileName, mode, stream);
			stdout_ex += 1;
			if (fc.fpStdout == NULL) 
				printf("[ERROR] opening file: '%s'",fileName);
		}else{
			fc.fpStderr = freopen(fileName, mode, stream);
			stderr_ex += 1;
			if (fc.fpStderr == NULL) 
				printf("[ERROR] opening file: '%s'",fileName);
		}
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
