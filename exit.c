#include "api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// print usage
int execute_exit_help(int internal){
	if(internal){
	    	printf("Usage : exit : exit program" H_RT);
		return 0;
	}
	
	printf(H_TAB "> exit : exit program" H_RT);
	return 0;
}

// deallocate resources and exit
int execute_exit(int argc, char* argv[]){
	// close
	if(fs->fd >= 0){
		close(fs->fd);
		ext2_fs_free(fs);
	}
	exit(0);
}
