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

#define NO_BRCH "┃"
#define BRCH "┣"
#define END_BRCH "┗"

#define TOPT_R (1)
#define TOPT_S (1 << 2)
#define TOPT_P (1 << 3)

/**
 * container for option flags for tree command
 */
int tree_opt;

// parse option flags using getopt
int parse_tree_opt(int argc, char *argv[]);

void print_info(const struct dirent *dir);

// print tree structure to stdout
int print_tree(const struct dirent *entry, const int depth, int last[], int *dir_count, int *file_count);
// void sort(struct dirent** entries, const int count);

// inline function to convert u32_t mode to string(drwxrwxrwx)
static inline void get_perm(u32_t mode, char *out)
{
	sprintf(out, "%s%s%s%s%s%s%s%s%s%s",
			(S_ISDIR(mode)) ? "d" : "-",
			(mode & S_IRUSR) ? "r" : "-",
			(mode & S_IWUSR) ? "w" : "-",
			(mode & S_IXUSR) ? "x" : "-",
			(mode & S_IRGRP) ? "r" : "-",
			(mode & S_IWGRP) ? "w" : "-",
			(mode & S_IXGRP) ? "x" : "-",
			(mode & S_IROTH) ? "r" : "-",
			(mode & S_IWOTH) ? "w" : "-",
			(mode & S_IXOTH) ? "x" : "-");
}

// print usage
// if internal, "Usage" is added, usually called when error occurred while executing this command
int execute_tree_help(int internal)
{
	if (internal)
	{
		printf("Usage : tree <PATH> [OPTION]... : display the directory structure if <PATH> is a directory" H_RT);
		printf(H_TAB "-r : display the directory structure recursively if <PATH> is a directory" H_RT);
		printf(H_TAB "-s : display the directory structure if <PATH> is a directory, including the size of each file" H_RT);
		printf(H_TAB "-p : display the directory structure if <PATH> is a directory, including the permissions of each directory and file" H_RT);
		return 0;
	}

	printf(H_TAB "> tree <PATH> [OPTION]... : display the directory structure if <PATH> is a directory" H_RT);
	printf(H_TTAB "-r : display the directory structure recursively if <PATH> is a directory" H_RT);
	printf(H_TTAB "-s : display the directory structure if <PATH> is a directory, including the size of each file" H_RT);
	printf(H_TTAB "-p : display the directory structure if <PATH> is a directory, including the permissions of each directory and file" H_RT);

	return 0;
}

// entry point of this command
// all tree command is executed here
// it accepts s, r, p options
// second argument must be directory name or root(.)
int execute_tree(int argc, char *argv[])
{
	struct dirent *root;
	// skip when no directory name entered
	if (argc < 2)
	{
		execute_tree_help(true);
		return -1;
	}

	char *fname = argv[1];

	// if . entered, treat it as root
	//	if(strcmp(fname, ".") == 0){
	//		root = root_dir;
	//	}else{
	char buf[PATH_MAX];
	memset(buf, 0, sizeof(char) * PATH_MAX);

	char filename[PATH_MAX];
	sprintf(filename, "%s", fname);

	int l = strlen(fname);
	// more flexible input handling
	// if a user enter dir ends with /, remove it
	if (filename[l - 1] == '/')
		filename[l - 1] = '\0';
	if (filename[0] != '.')
	{
		sprintf(filename, "./%s", fname);
	}
	// search ext2 file system
	root = ext2_find_dirent(filename, buf, root_dir);
	//	}

	// if unavailable option found
	if (parse_tree_opt(argc, argv) < 0)
	{
		execute_tree_help(true);
		return -1;
	}

	// no directory found
	if (root == NULL)
	{
		fprintf(stderr, "%s does not exist.\n", fname);
		return -1;
	}

	// input path is not directory
	if (!S_ISDIR(root->inode.i_mode))
	{
		fprintf(stderr, "Error: '%s' is not directory\n", fname);
		return -1;
	}

	int last_buf[256] = {0};
	int dir_count = 1;
	int file_count = 0;

	if (tree_opt & TOPT_S || tree_opt & TOPT_P)
		print_info(root);

	printf("%s\n", fname);

	// execute tree
	if (print_tree(root, 0, last_buf, &dir_count, &file_count) < 0)
	{
		return -1;
	}

	printf("\n%d directories, %d files\n", dir_count, file_count);

	return 0;
}

// print additional information of entry
// size and permission(drwxrwxrwx)
// only available when at least one of s,p option is entered
void print_info(const struct dirent *dir)
{
	char perm_info[256];
	char size_info[256];

	if (!(tree_opt & TOPT_S || tree_opt & TOPT_P))
		return;

	printf("[");

	if (tree_opt & TOPT_P)
	{
		get_perm(dir->inode.i_mode, perm_info);
		printf("%s", perm_info);
		if (tree_opt & TOPT_S)
			printf(" ");
	}

	if (tree_opt & TOPT_S)
	{
		sprintf(size_info, "%llu", dir->inode.i_size);
		printf("%s", size_info);
	}

	printf("]");
}

// if -r option is set, it recursively searches children until ends
// else it only searches first depth entries
int print_tree(const struct dirent *dir, const int depth, int last[], int *dir_count, int *file_count)
{
	for (int i = 0; i < dir->count; i++)
	{
		struct dirent ent = dir->children[i];

		if (S_ISDIR(ent.inode.i_mode))
		{
			(*dir_count)++;
		}
		else
		{
			(*file_count)++;
		}

		for (int j = 0; j < depth; j++)
		{
			if (last[j])
			{
				printf("  ");
			}
			else
			{
				printf(NO_BRCH "  ");
			}
		}

		// last entry
		if (i + 1 == dir->count)
		{
			printf(END_BRCH " ");
		}
		else
		{
			printf(BRCH " ");
		}

		print_info(&ent);

		printf(" %s\n", ent.name);

		// if entry is directory and -r option set, recursively search
		if (S_ISDIR(ent.inode.i_mode) && (tree_opt & TOPT_R))
		{
			last[depth] = (i + 1 == dir->count);
			print_tree(&ent, depth + 1, last, dir_count, file_count);
		}
	}

	return 0;
}

int parse_tree_opt(int argc, char *argv[])
{
	tree_opt = 0;
	optind = 0;
	int opt;

	while ((opt = getopt(argc, argv, "rsp")) != -1)
	{
		switch (opt)
		{
		case 'r':
			tree_opt |= TOPT_R;
			break;
		case 's':
			tree_opt |= TOPT_S;
			break;
		case 'p':
			tree_opt |= TOPT_P;
			break;
		default:
			return -1;
		}
	}

	return 0;
}