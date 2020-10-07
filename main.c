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

#include "debug.h"
#include "memory.h"
#include "args.h"

struct sigaction act_info;

void executeLinesFromFile(char *sFile);
void makeSignalFile();
void takeCareOfSignalInfo(int signal, siginfo_t *siginfo, void *context);

int main(int argc, char *argv[]){
	
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
		ERROR(4, "sigaction (SIGINT) - ???");
	if(sigaction(SIGUSR1, &act_info, NULL) < 0)
		ERROR(5, "sigaction (SIGUSR1) - ???");
	if(sigaction(SIGIUSR2, &act_info, NULL) < 0)
		ERROR(6, "sigaction (SIGIUSR2) - ???");
			


	
	// gengetopt: release resources
	//cmdline_parser_free(&args);


	return 0;
}

/*
	Opens the file

	return FILE
*/
void executeLinesFromFile(char *sFile){
	//FALTA MUDAR A MSG DE ERRO E MOSTRAR O ERRO DE SISTEMA
	FILE *file = fopen(sFile, "r");
    if (file == NULL)
        ERROR(3, "[Error]: file not found.\n");
		//CONFIRMAR SE ISTO ACABA MSM COM O PROGRAMA
	
	/*char line[256];
    while (fgets(line, sizeof(line), file)) {
        printf("%s", line);
		printf("%ld", sizeof(line)); 
    }
	*/
	char * line = NULL;
    size_t len = 0;
    ssize_t read;
	while ((read = getline(&line, &len, file)) != -1) {
		if(read != 1 && line[0] != '#'){
			printf("Retrieved line of length %zu:\n", read);
			printf("%s", line);
		}
    }
}

void makeSignalFile(){
	FILE *outputFile = fopen("signal.txt", "wab+");
	fprintf(outputFile, "kill -SIGINT %d\nkill -SIGUSR1 %d\nkill -SIGIUSR2 %d",getpid(),getpid(),getpid());
	fclose(outputFile);
	printf("[INFO] created file  '%s'\n", "signal.txt");
}

void takeCareOfSignalInfo(int signal, siginfo_t *siginfo, void *context) {
	(void)context;
	/* Cópia da variável global errno */
	aux = errno;


	printf("Recebi o sinal (%d)\n", signal);
	printf("Detalhes:\n");
	printf("\tPID: %ld\n\tUID: %ld\n", (long)siginfo->si_pid, (long)siginfo->si_uid);


	/* Restaura valor da variável global errno */
	errno = aux;
}

