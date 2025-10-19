#ifndef _API_H_
#define _API_H_
#include "ext2layer.h"

#define H_TAB "   "
#define H_TTAB "     "
#define H_RT "\n"

// container struct for storing command information
// execute function, usage print function and name of the command which is identifier in prompt
struct command {
	// when name of this command entered in prompt, this function is called with arguments
	int (*execute)(int, char**);
	// when help executed or unrecognized command encountered, this function is called to print usage
	// if printing internal usage, parameter is 1, else 0 
	int (*help)(int);
	// label, or name of command, used to identify command
	char name[20];
};

// extern variable for accessing ext2 file system
// ext2_fs is defined as struct _ext2_fs*
extern ext2_fs fs;

// extern variable for accessing root entry of ext2 file system
// all operations regarding ext2 file system start from root directory
extern struct dirent* root_dir;

// array of commands that are recognized in prompt
extern struct command *commands;

// reference for tree command executor
extern int execute_tree(int argc, char* argv[]);
// reference for usage printer for tree command
extern int execute_tree_help(int internal);

// reference for print command executor
extern int execute_print(int argc, char* argv[]);
// reference for usage printer for print command
extern int execute_print_help(int internal);

// reference for help command executor
extern int execute_help(int argc, char* argv[]);
// reference for usage printer for help command
extern int execute_help_help(int internal);

// reference for exit command executor
extern int execute_exit(int argc, char* argv[]);
// reference for usage printer for exit command
extern int execute_exit_help(int internal);

#endif
