#include "api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "ext2layer.h"
#define INCLUDE_HOME_DIR
#define BUFFER_SIZE 1024

ext2_fs fs;
struct dirent *root_dir;
struct command *commands;

// split string by delimiter and create array of strings
char **tokenize_args(char *str, int *argc, char *del)
{
	*argc = 0;
	char *tmp[100] = {
		(char *)NULL,
	};
	char *tok = NULL;
	tok = strtok(str, del);
	while (tok != NULL)
	{
		tmp[(*argc)++] = tok;
		tok = strtok(NULL, del);
	}

	char **argv = (char **)malloc(sizeof(char *) * (*argc + 1));
	for (int i = 0; i < *argc; i++)
	{
		argv[i] = tmp[i];
	}

	return argv;
}

// check give command is available and call command
int handle_cmd(char *all, int argc, char **argv);

// open ext2 fs image file and load onto memory
// after ext2 has been opened, it scans all entries in fs and create linked list of entry
// print, tree command depends on this
int init(const char *imgfile)
{
	if (ext2_fs_open(imgfile, &fs) < 0)
		return -1;

	if (fs->group_desc_count < 1)
		return -1;

	struct dirent *root = (struct dirent *)calloc(1, sizeof(struct dirent));

	struct ext2_group_desc desc = fs->group_desc[0];
	u32_t inode_offset = desc.bg_inode_table;

	struct dirent tmp;
	memset(&tmp, 0, sizeof(struct dirent));
	int c = ext2_fs_scan(fs, inode_offset, &tmp);
	root->count = tmp.count;
	strcpy(root->name, tmp.name);
	ext2_copy_inode(&root->inode, tmp.inode);
	root->children = tmp.children;
	root_dir = root;

	// no entry found
	if (root_dir == NULL)
		return -1;

	return 0;
}

// init commands - locate executor, usage printer and name
void init_commands()
{
	commands = (struct command *)calloc(4, sizeof(struct command));

	strcpy(commands[0].name, "tree");
	commands[0].execute = execute_tree;
	commands[0].help = execute_tree_help;
	strcpy(commands[1].name, "print");
	commands[1].execute = execute_print;
	commands[1].help = execute_print_help;
	strcpy(commands[2].name, "help");
	commands[2].execute = execute_help;
	commands[2].help = execute_help_help;
	strcpy(commands[3].name, "exit");
	commands[3].execute = execute_exit;
	commands[3].help = execute_exit_help;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage Error : %s <EXT2_IMAGE>\n", argv[0]);
		exit(1);
	}

	if (init(argv[1]) < 0)
	{
		exit(1);
	}

	init_commands();

	char in[BUFFER_SIZE];
	int _argc;
	char **_argv = NULL;

	while (true)
	{
		printf("20211430> ");
		fgets(in, BUFFER_SIZE, stdin);
		in[strlen(in) - 1] = '\0';
		if ((_argv = tokenize_args(in, &_argc, " ")) == NULL)
			continue;
		int r = handle_cmd(in, _argc, _argv);

		free(_argv);
	}

	// free
	if (fs->fd >= 0)
	{
		close(fs->fd);
		ext2_fs_free(fs);
	}

	exit(0);
}

void print_help()
{
	printf("Usage : \n");
	for (int i = 0; i < 4; i++)
	{
		commands[i].help(false);
	}
}
int handle_cmd(char *all, int argc, char **argv)
{
	if (argc < 1)
	{
		print_help();
		return 0;
	}

	for (int i = 0; i < 4; i++)
	{
		if (strcmp(commands[i].name, argv[0]) == 0)
		{
			return commands[i].execute(argc, argv);
		}
	}

	print_help();

	return 0;
}
