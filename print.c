#include "api.h"
#include "ext2layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <stdbool.h>

#define POPT_N (1)

// container for option flags for print
int print_opt;

// if availble, argument of -n stored
u32_t lines;

// parse option flags
int parse_print_opt(int argc, char *argv[]);

// print usage
int execute_print_help(int internal)
{
	if (internal)
	{
		printf("Usage : print <PATH> [OPTION]... : print the contents on the standard output if <PATH> is file" H_RT);
		printf(H_TAB "-n <line_number> : print only the first <line_number> lines of its contents on the standard output if <PATH> is file" H_RT);
		return 0;
	}

	printf(H_TAB "> print <PATH> [OPTION]... : print the contents on the standard output if <PATH> is file" H_RT);
	printf(H_TTAB "-n <line_number> : print only the first <line_number> lines of its contents on the standard output if <PATH> is file" H_RT);

	return 0;
}

// execute print, called by prompt
int execute_print(int argc, char *argv[])
{
	struct dirent *target;

	// if no path entered
	if (argc < 2)
	{
		execute_print_help(true);
		return -1;
	}

	// same as tree command, we treat . as root entry
	// but in print command, only non directory is accepted
	// so this is meaningless
	char *fname = argv[1];
	// if(strcmp(fname, ".") == 0){
	// target = root_dir;
	// }else{
	char filename[PATH_MAX];
	sprintf(filename, "%s", fname);
	if (filename[0] != '.')
	{
		sprintf(filename, "./%s", fname);
	}
	char pbuf[PATH_MAX];
	memset(pbuf, 0, sizeof(char) * PATH_MAX);
	// find entry by name
	target = ext2_find_dirent(filename, pbuf, root_dir);
	// }

	if (parse_print_opt(argc, argv) < 0)
	{
		//	execute_print_help(true);
		return -1;
	}

	if (target == NULL)
	{
		fprintf(stderr, "%s does not exist.\n", fname);
		return -1;
	}

	// accepts only nondirectory file
	if (S_ISDIR(target->inode.i_mode))
	{
		fprintf(stderr, "Error: '%s' is not file\n", fname);
		return -1;
	}

	u64_t len;
	char *buf = (char *)calloc(target->inode.i_size, sizeof(char));
	// read data of given size from datablock which inode points
	if ((len = ext2_read(fs, target->inode, buf, target->inode.i_size)) < 0)
	{
		len = 0;
	}
	buf[len] = '\0';

	u64_t n = 0;
	for (u64_t i = 0; i < len; i++)
	{
		// if -n option is set, we cut printing at exact line
		if ((print_opt & POPT_N) && n >= lines)
			break;

		char c = buf[i];
		if (c == '\n')
			n++;

		fputc(c, stdout);
	}

	fputc('\n', stdout);

	free(buf);
	return 0;
}

// parse options
int parse_print_opt(int argc, char *argv[])
{
	print_opt = 0;
	optind = 0;
	int opt;

	while ((opt = getopt(argc, argv, "n:")) != -1)
	{
		switch (opt)
		{
		case 'n':
			print_opt |= POPT_N;
			lines = (u32_t)strtol(optarg, NULL, 10);
			break;
		default:
			return -1;
		}
	}

	return 0;
}
