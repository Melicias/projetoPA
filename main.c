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

#include "debug.h"
#include "memory.h"
#include "args.h"

struct sigaction act_info;
struct tm tm;

time_t t;


void executeLinesFromFile(char *sFile);
void makeSignalFile();
void takeCareOfSignalInfo(int signal, siginfo_t *siginfo, void *context);

int main(int argc, char *argv[]){
	
	t = time(NULL);
	tm = *localtime(&t);

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
		ERROR(5, "sigaction (SIGUSR1) testeeeeeeee");
	if(sigaction(SIGUSR2, &act_info, NULL) < 0)
		ERROR(6, "sigaction (SIGUSR2)");
			
	sleep(100);

	
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
	printf("[INFO] executing from file '%s'",sFile);
	while ((read = getline(&line, &len, file)) != -1) {
		if(read != 1 && line[0] != '#'){
			//fazer os prints com as cenas necessarias para por a shell a funcionar
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
	
	switch(signal){
		case 2: //SIGINT 
			printf("[INFO]\tPID who send the signal: %ld \n\tUID who send the signal: %ld\n", (long)siginfo->si_pid, (long)siginfo->si_uid);
			break;
		case 10: //SIGUSR1 
			printf("[INFO] nanoShell started at: %d-%02d-%02dT%02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			break;
		case 12: ;//SIGUSR2
			char fileName[50];
			sprintf(fileName,"nano_shell_status_%d.%02d.%02d_%02dh%02d.%02d.txt", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			FILE *outputFile = fopen(fileName, "wab+");
			fprintf(outputFile, "this is a test"); //adicionar aqui as cenas do output do file
			fclose(outputFile);
			printf("[INFO] created file  '%s'\n", fileName);
			break;
		default:
			printf("teste");

	}

	/* Restaura valor da variável global errno */
	errno = aux;
}

/*
bool checkcheckIfCommandIsValid(char* command){

}

void runShellCommand(char* command){
	if(!checkIfCommandIsValid(command)){
		//ERRO DAR HANDLE NISSO
	}

	pid_t pid = fork();
	if (pid == 0) {			// Processo filho 
		

		exit(0);
	} else if (pid > 0) {	// Processo pai 
		wait(NULL);
	} else					// < 0 - erro
		ERROR(2, "Erro na execucao do fork()");
}*/
