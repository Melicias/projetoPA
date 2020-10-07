/**
* @file main.c
* @brief Description
* @date 2020-10-6
* @author Francisco Melicias - 2191727 & Diogo Francisco - 219----
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#include "debug.h"
#include "memory.h"
#include "args.h"


void executeLinesFromFile(char *sFile);
void makeSignalFile();

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
	FILE *outputFile = outputFile = fopen("signal.txt", "wab+");
	printf("Writting the output into the file: %s\n", "signal.txt");
	fprintf(outputFile, "kill -SIGINT %d\nkill -SIGUSR1 %d\nkill -SIGIUSR2 %d",getpid(),getpid(),getpid());
	fclose(outputFile);
}

