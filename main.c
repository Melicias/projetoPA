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


int main(int argc, char *argv[]){
	
	struct gengetopt_args_info args;

	// gengetopt parser
	if(cmdline_parser(argc, argv, &args))
		ERROR(1, "[Error]: cmdline_parser execution.\n");

	// all the option only must be used alone, so 1 is always from the name of file
	if(argc > 2)
		ERROR(2, "[Error]: All the option must to be used on their own.\n");

	// gengetopt: release resources
	//cmdline_parser_free(&args);


	return 0;
}

