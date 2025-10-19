#ifndef _EXT2_LAYER_H_
#define _EXT2_LAYER_H_

#include "base.h"
#include <string.h>

// name length of each entry in ext2
#define EXT2_NAME_LEN 255
// magic number of ext2
#define EXT2_SUPER_MAGIC 0xEF53
// root directory inode number
#define EXT2_ROOT_INO 2

// file types of ext2
// unknown
#define EXT2_FILE_UNK 0x0
// regular file
#define EXT2_FILE_REG 0x1
// directory
#define EXT2_FILE_DIR 0x2
// character device
#define EXT2_FILE_CHR 0x3
// block
#define EXT2_FILE_BLK 0x4
// fifo
#define EXT2_FILE_FIFO 0x5
// socket
#define EXT2_FILE_SOCK 0x6
// symbolic link
#define EXT2_FILE_LNK 0x7

typedef unsigned int u32_t;
typedef unsigned short u16_t;
typedef short s16_t;
typedef unsigned char u8_t;
typedef unsigned long long u64_t;
// for calculating size of group descriptor
// not used for reading operation
struct _ext2_group_desc
{
	u32_t bg_block_bitmap;		/* Blocks bitmap block */
	u32_t bg_inode_bitmap;		/* Inodes bitmap block */
	u32_t bg_inode_table;		/* Inodes table block */
	u16_t bg_free_blocks_count; /* Free blocks count */
	u16_t bg_free_inodes_count; /* Free inodes count */
	u16_t bg_used_dirs_count;	/* Directories count */
	u16_t bg_flags;
	u32_t bg_reserved[2];
	u16_t bg_itable_unused; /* Unused inodes count */
	u16_t bg_checksum;		/* crc16(s_uuid+grouo_num+group_desc)*/
};

// directory entry struct in directory
struct ext2_dir_entry
{
	u32_t inode;   // le
	u16_t rec_len; // le
	u8_t name_len;
	u8_t file_type;
	char name[EXT2_NAME_LEN];
};

// inode struct
struct ext2_inode
{
	u16_t i_mode;	   // le
	u64_t i_size;	   // le
	u32_t i_blocks;	   // le
	u32_t i_block[15]; // le
};

// block group descriptor struct
struct ext2_group_desc
{
	// location of inode table in block group
	u32_t bg_inode_table;
	u16_t bg_used_dirs_count;
};

// super block struct of super block in block group
struct ext2_super_block
{
	u32_t s_inodes_count;
	u32_t s_blocks_count;
	u32_t s_first_data_block;
	u32_t s_log_block_size;
	u32_t s_blocks_per_group;
	u32_t s_inodes_per_group;
	u16_t s_magic;
	u32_t s_first_ino;
	u16_t s_inode_size;
};

// core ext2 file system struct for accessing significant properties
struct _ext2_fs
{
	// file descriptor of ext2 image file
	// must be closed when resource deallocation stage
	int fd;

	// super block
	struct ext2_super_block *super;
	// block group descriptor list
	struct ext2_group_desc *group_desc;
	// count of block groups
	unsigned int group_desc_count;
	unsigned long desc_blocks;
	// size of block, defaults to 0x1000
	u32_t block_size;
};

// directory entry in directory
struct dirent
{
	u64_t count;
	char name[EXT2_NAME_LEN];
	struct ext2_inode inode;

	struct dirent *parent;
	struct dirent *children;
};

struct ext2_datablock
{
	struct ext2_inode inode;
	u32_t block_idx;
	u32_t block_offset;
	u32_t d_block_offset;
	u32_t dd_block_offset;
};

typedef struct _ext2_fs *ext2_fs;

// open ext2 and read properties of ext2 fs
// also read block group descriptors
// all results are saved to out
extern int ext2_fs_open(const char *filename, ext2_fs *out);

// must be called only once, starting from root directory it recursively scans child entries to build linked tree which represents file system hierarchies
// ret_dirent would be root entry and all cascading entries are children
extern int ext2_fs_scan(ext2_fs, u32_t inode_table, struct dirent *ret_dirent);

// read inode struct at inode table with offset to inode pointer
extern int ext2_fs_read_inode(ext2_fs fs, u32_t inode_table, u32_t num, struct ext2_inode *inode);

// read directory entry at given position
extern u64_t ext2_fs_read_dirent(ext2_fs fs, u64_t offset, struct ext2_dir_entry *entry);

// scan all directory entries within given directory
// block and inode should point directory
extern u64_t ext2_fs_scan_dir(ext2_fs fs, u32_t block, struct ext2_inode inode, struct ext2_dir_entry **entries);

// deallocate resource
extern void ext2_fs_free(ext2_fs fs);

// find file entry matching with given path
// pass char buffer with enough size so that it stores parent's path and more
// everytime it looks into deeper entry, it stacks name of entry delimited by /
// then when buffer equals with path, it returns that entry
// or no entry found after scanning completes, it returns null
extern struct dirent *ext2_find_dirent(const char *path, char *buffer, const struct dirent *ent);

// read data block with given size
// it reads through data blocks which inode points
extern u64_t ext2_read(const ext2_fs fs, const struct ext2_inode inode, char *out, const u64_t size);

// begin reading datablocks
extern int ext2_begin_datablock(const ext2_fs fs, const struct ext2_inode inode, struct ext2_datablock *out);

// get next position of datablock currently available
// if next datablock is indirect block or double indirect block,
// return exact location of datablock which holds real data by following indirect addresses
// return 0 if there's no more datablock
extern u32_t ext2_next_datablock(const ext2_fs fs, struct ext2_datablock *blk);

// inline function which copies inode struct
static inline void ext2_copy_inode(struct ext2_inode *dest, const struct ext2_inode src)
{
	memset(dest, 0, sizeof(struct ext2_inode));
	memcpy(dest, &src, sizeof(struct ext2_inode));
}

static inline unsigned int ext2fs_div_ceil(unsigned int a, unsigned int b)
{
	if (!a)
		return 0;
	return ((a - 1) / b) + 1;
}

#endif
