#include "api.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// print usage of help command
int execute_help_help(int internal){
	if(internal){
		printf("Usage : help [COMMAND] : show commands for program" H_RT);
		return 0;
	}

	printf(H_TAB "> help [COMMAND] : show commands for program" H_RT);
	return 0;
}

// print all usages if no command provided
// or print specific usage of that command
int execute_help(int argc, char* argv[]){
	if(argc < 2){
		printf("Usage : \n");
		for(int i = 0; i < 4; i++){
			commands[i].help(false);
		}
		return 0;
	}

	char* cmd = argv[1];

	for(int i = 0; i < 4; i++){
		if(strcmp(cmd, commands[i].name) == 0){
			commands[i].help(true);
			return 0;
		}
	}
	
	printf("invalid command -- '%s'\n", cmd);
	printf("Usage : \n");
	for(int i = 0; i < 4; i++){
			commands[i].help(false);
	}
	return 0;

}
