#include "ext2layer.h"
#include "memhelper.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <linux/limits.h>

// open ext2 image file and read super block, group descriptor
// it simply reads properties and descriptors, no datablock read operation occurred
// like dump2efs
int ext2_fs_open(const char *filename, ext2_fs *out)
{
	ext2_fs fs;
	int fd;
	int ret;

	// open
	if ((fd = open(filename, O_RDONLY)) < 0)
	{
		return -1;
	}

	// malloc struct _ext2_fs
	ret = memhelper_get(sizeof(struct _ext2_fs), (void **)&fs);
	if (ret)
		return ret;
	memset(fs, 0, sizeof(struct _ext2_fs));

	fs->fd = fd;

	ret = memhelper_get(sizeof(struct ext2_super_block), (void **)&fs->super);
	if (ret)
	{
		goto cleanup;
	}

	lseek(fd, 1024, SEEK_SET);

	// read super block
	// read inodes count
	read(fd, &fs->super->s_inodes_count, sizeof(u32_t));
	// read blocks count
	read(fd, &fs->super->s_blocks_count, sizeof(u32_t));

	lseek(fd, 3 * sizeof(u32_t), SEEK_CUR); // move to s_first_data_block

	// read first data block
	read(fd, &fs->super->s_first_data_block, sizeof(u32_t));
	// read block size
	read(fd, &fs->super->s_log_block_size, sizeof(u32_t));

	// calculate block size
	fs->block_size = 1024 << fs->super->s_log_block_size;

	lseek(fd, sizeof(u32_t), SEEK_CUR); // move to s_blocks_per_group

	// read blocks per group
	read(fd, &fs->super->s_blocks_per_group, sizeof(u32_t));

	lseek(fd, sizeof(u32_t), SEEK_CUR); // move to s_inodes_per_group

	// read inodes per group
	read(fd, &fs->super->s_inodes_per_group, sizeof(u32_t));

	// move to s_magic
	lseek(fd, 2 * sizeof(u32_t) + sizeof(u16_t) + sizeof(s16_t), SEEK_CUR);

	// read magic number
	read(fd, &fs->super->s_magic, sizeof(u16_t));

	// if magic number does not match, it is not a ext2 formatted or corrupted
	if (fs->super->s_magic != EXT2_SUPER_MAGIC)
	{
		ret = -1;
		goto cleanup;
	}

	lseek(fd, 5 * sizeof(u16_t) + 4 * sizeof(u32_t), SEEK_CUR); // move to first_ino

	// read first ino
	read(fd, &fs->super->s_first_ino, sizeof(u32_t));
	// read inode size
	read(fd, &fs->super->s_inode_size, sizeof(u16_t));

	// get struct size of group descriptor to get next offset of group descriptor struct
	u32_t desc_size = sizeof(struct _ext2_group_desc);
	u32_t desc_per_block = fs->block_size / desc_size;

	// caculate count of group descriptor block
	fs->group_desc_count = (fs->super->s_blocks_count - fs->super->s_first_data_block + fs->super->s_blocks_per_group - 1) / fs->super->s_blocks_per_group;
	// create group descriptor array
	fs->group_desc = (struct ext2_group_desc *)calloc(fs->group_desc_count, sizeof(struct ext2_group_desc));

	// group descriptor start at 0x1000 * block size
	off_t gdt_start = fs->block_size;
	if (fs->block_size == 1024)
	{
		gdt_start = fs->block_size * 2;
	}
	// read all group descriptor
	for (u32_t i = 0; i < fs->group_desc_count; i++)
	{
		struct ext2_group_desc *desc = &fs->group_desc[i];
		lseek(fd, gdt_start + i * desc_size, SEEK_SET);

		// move to inode table
		lseek(fd, 2 * sizeof(u32_t), SEEK_CUR);
		// read inode table block location
		read(fd, &desc->bg_inode_table, sizeof(u32_t));

		// move to used dirs count
		lseek(fd, 2 * sizeof(u32_t), SEEK_CUR);
		// read used dirs count
		read(fd, &desc->bg_used_dirs_count, sizeof(u16_t));
	}

	*out = fs;
	return 0;
cleanup:

	ext2_fs_free(fs);
	return ret;
}

// read inode from data block
// it calculates group number from ino, so inode_start is not used, just for legacy
// i don't know why but there is no need to manually handle little endian format
// read() automatically convert them i guess
int ext2_fs_read_inode(ext2_fs fs, u32_t inode_start, u32_t ino, struct ext2_inode *inode)
{
	if (inode == NULL)
		return -1;

	if (fs->fd < 0)
		return -1;

	int fd = fs->fd;

	u32_t group = (ino - 1) / fs->super->s_inodes_per_group;
	u32_t offset = (ino - 1) % fs->super->s_inodes_per_group;
	inode_start = fs->group_desc[group].bg_inode_table;

	memset(inode, 0, sizeof(struct ext2_inode));
	// go to inode table block

	u64_t loc = (u64_t)inode_start * (u64_t)fs->block_size + (u64_t)offset * (u64_t)fs->super->s_inode_size;
	lseek(fd, loc, SEEK_SET);
	// read mode
	read(fd, &inode->i_mode, sizeof(u16_t));
	// inode->i_mode = swap16(inode->i_mode);

	// read size
	lseek(fd, sizeof(u16_t), SEEK_CUR);
	u32_t size = 0;
	read(fd, &size, sizeof(u32_t));
	inode->i_size = size;
	// inode->i_size = swap32(inode->i_size);

	// read count of blocks
	lseek(fd, 4 * sizeof(u32_t) + 2 * sizeof(u16_t), SEEK_CUR);
	read(fd, &inode->i_blocks, sizeof(u32_t));
	// inode->i_blocks = swap32(inode->i_blocks);

	// read all blocks
	lseek(fd, 2 * sizeof(u32_t), SEEK_CUR);
	memset(&inode->i_block[0], 0, sizeof(u32_t) * 15);
	read(fd, &inode->i_block[0], sizeof(u32_t) * 15);

	// read size high if possible
	lseek(fd, sizeof(u32_t) * 2, SEEK_CUR);
	u32_t size_high = 0;
	read(fd, &size_high, sizeof(u32_t));

	if (size_high > 0)
	{
		inode->i_size |= (u64_t)size_high << 32;
	}

	return inode->i_blocks;
}

// read one entry in a directory
u64_t ext2_fs_read_dirent(ext2_fs fs, u64_t offset, struct ext2_dir_entry *entry)
{
	if (entry == NULL)
		return -1;

	if (fs->fd < 0)
		return -1;

	int fd = fs->fd;
	memset(entry, 0, sizeof(struct ext2_dir_entry));
	lseek(fd, offset, SEEK_SET);
	read(fd, &entry->inode, sizeof(u32_t));
	//	entry->inode = swap32(entry->inode);

	read(fd, &entry->rec_len, sizeof(u16_t));
	//	entry->rec_len = swap16(entry->rec_len);
	read(fd, &entry->name_len, sizeof(u8_t));
	read(fd, &entry->file_type, sizeof(u8_t));

	read(fd, &entry->name[0], sizeof(char) * entry->name_len);
	entry->name[entry->name_len] = '\0';
	return (u64_t)entry->rec_len;
}

// get all directory entries by accessing to data block
// it does not recursively scan, only fetch direct child entries
u64_t ext2_fs_scan_dir(ext2_fs fs, u32_t block_idx, struct ext2_inode inode, struct ext2_dir_entry **ret_entries)
{
	if (ret_entries == NULL)
		return -1;

	if (fs->fd < 0)
		return -1;

	int fd = fs->fd;
	u32_t blk = block_idx;
	u64_t offset = (u64_t)inode.i_block[blk] * (u64_t)fs->block_size;
	struct ext2_dir_entry entry;

	u64_t count = 0;
	u64_t length = 0;

	struct ext2_datablock db;
	if (ext2_begin_datablock(fs, inode, &db) < 0)
		return -1;

	u64_t pos = 0;
	while ((pos = ext2_next_datablock(fs, &db)) != 0)
	{
		offset = pos * (u64_t)fs->block_size;
		while (length < (u64_t)fs->block_size)
		{
			u64_t l = ext2_fs_read_dirent(fs, offset + length, &entry);
			if (l < 1)
				break;

			length += l;
			count += 1;
		}

		length = 0;
	}

	struct ext2_dir_entry *entries = (struct ext2_dir_entry *)calloc(count, sizeof(struct ext2_dir_entry)); // create entry list

	u64_t i = 0;
	length = 0;
	offset = 0;

	memset(&db, 0, sizeof(struct ext2_datablock));
	if (ext2_begin_datablock(fs, inode, &db) < 0)
		return -1;

	while ((pos = ext2_next_datablock(fs, &db)) != 0)
	{
		offset = pos * (u64_t)fs->block_size;
		while (length < (u64_t)fs->block_size)
		{
			memset(&entry, 0, sizeof(struct ext2_dir_entry));
			u64_t l = ext2_fs_read_dirent(fs, offset + length, &entry);
			if (l < 1)
				break;

			entries[i].inode = entry.inode;
			entries[i].rec_len = entry.rec_len;
			entries[i].name_len = entry.name_len;
			entries[i].file_type = entry.file_type;
			strcpy(entries[i].name, entry.name);
			length += l;
			i++;
		}

		length = 0;
	}

	*ret_entries = entries;
	return count;
}

// recursively scan directories
// it calls ext2_fs_scan_dir to list current direct child entries
// and if a child type is directory, it selects and calls itself to deeper entries
// used to build a whole tree representing ext2 file system
u64_t ext2_fs_scandir(ext2_fs fs, u32_t inode_table, u8_t filetype, struct ext2_inode inode, struct dirent *parent, struct dirent *ret_dirent)
{
	struct ext2_dir_entry *entries;
	if (!S_ISDIR(inode.i_mode))
	{
		return 0;
	}

	u64_t c = ext2_fs_scan_dir(fs, 0, inode, &entries);
	u64_t count = 0;
	struct dirent *children = (struct dirent *)calloc(c, sizeof(struct dirent));
	u64_t j = 0;
	for (u64_t i = 0; i < c; i++)
	{
		struct ext2_dir_entry entry = entries[i];
		if (strcmp(entry.name, ".") && strcmp(entry.name, "..") && strcmp(entry.name, "lost+found"))
		{
			struct ext2_inode temp;
			if (ext2_fs_read_inode(fs, inode_table, entry.inode, &temp) < 0)
				continue;

			struct dirent *child = &children[j++];
			sprintf(child->name, "%s", entry.name);
			ext2_copy_inode(&child->inode, temp);
			ext2_fs_scandir(fs, inode_table, entry.file_type, child->inode, ret_dirent, child);
			count++;
		}
	}

	ret_dirent->children = children;
	ret_dirent->count = count;
	ret_dirent->parent = parent;
	free(entries);
	return count;
}

// from the root, it finds all entries by building trees by calling ext2_fs_scandir
// starting from read inode of root entry of ext2 file system, it goes down to leaf children
int ext2_fs_scan(ext2_fs fs, u32_t inode_table, struct dirent *ret_dirent)
{
	struct ext2_inode inode;
	if (ext2_fs_read_inode(fs, inode_table, EXT2_ROOT_INO, &inode) < 0)
		return -1;

	ext2_copy_inode(&ret_dirent->inode, inode);
	strcpy(ret_dirent->name, ".");
	return (int)ext2_fs_scandir(fs, inode_table, EXT2_FILE_DIR, inode, NULL, ret_dirent);
}

// find a file entry from dirent tree built by ext2_fs_scan
// everytime it enters directory, concatenating two entry name by / allows us to build path
// to compare with input path
// buffer holds this value to compare with path
struct dirent *ext2_find_dirent(const char *path, char *buffer, const struct dirent *ent)
{
	char cwp[PATH_MAX];

	if (strlen(buffer) < 1)
		sprintf(cwp, "%s", ent->name);
	else
		sprintf(cwp, "%s/%s", buffer, ent->name);
	if (strcmp(cwp, path) == 0)
	{
		return (struct dirent *)ent;
	}

	if (!S_ISDIR(ent->inode.i_mode))
		return NULL;

	for (int i = 0; i < ent->count; i++)
	{
		struct dirent e = ent->children[i];
		struct dirent *ret = ext2_find_dirent(path, cwp, &e);
		if (ret != NULL)
			return ret;
	}

	return NULL;
}

// read bytes at given datablocks pointed by inode, allowing indirect and double indirect blocks
// it does not read more than given size
u64_t ext2_read(const ext2_fs fs, const struct ext2_inode inode, char *out, const u64_t size)
{
	char *p = out;
	u64_t bytes = inode.i_size > size ? size : inode.i_size;
	u64_t bytes_left = bytes;

	struct ext2_datablock db;
	if (ext2_begin_datablock(fs, inode, &db) < 0)
		return -1;

	while (bytes_left > 0)
	{
		u32_t block_num = ext2_next_datablock(fs, &db);
		if (block_num == 0)
			break;

		u64_t to_read = fs->block_size > bytes_left ? bytes_left : fs->block_size;
		lseek(fs->fd, block_num * fs->block_size, SEEK_SET);
		ssize_t read_bytes = read(fs->fd, p, to_read);
		if (read_bytes < 0)
			break;

		p += read_bytes;
		bytes_left -= read_bytes;
	}

	return bytes - bytes_left;
}

// initialize datablock struct which holds current data block position we look for
// block_offset is used when fetching indirect blocks
// d_block_offset is used when fetching double indirect blocks
// datablock struct provides helper tools to automatically handle indirect blocks if required
extern int ext2_begin_datablock(const ext2_fs fs, const struct ext2_inode inode, struct ext2_datablock *out)
{
	// inode has no block
	if (inode.i_blocks < 1)
	{
		return -1;
	}
	out->inode = inode;
	out->block_idx = 0;
	out->block_offset = 0;
	out->d_block_offset = 0;
	out->dd_block_offset = 0;
	return 0;
}

// get next datablock position
// if we are currently looking at indirect block, it references to real data block position
// so the caller of this method doesn't have to care about indirect blocks
extern u32_t ext2_next_datablock(const ext2_fs fs, struct ext2_datablock *blk)
{
	struct ext2_inode inode;
	inode = blk->inode;
	// no blocks
	if (!inode.i_block[blk->block_idx])
	{
		return 0;
	}

	// handle direct block
	if (blk->block_idx < 12)
	{
		return inode.i_block[blk->block_idx++];
	}

	// handle indirect block
	if (blk->block_idx == 12)
	{
		// go to block location and read int(4-bytes), and the result will be actual location of block
		u32_t p = 0;
		lseek(fs->fd, inode.i_block[blk->block_idx] * fs->block_size + blk->block_offset, SEEK_SET);
		blk->block_offset += read(fs->fd, &p, sizeof(u32_t));

		// end of block
		if (blk->block_offset >= fs->block_size)
		{
			blk->block_offset = 0;
			blk->block_idx++;
		}

		return p;
	}

	// handle double indirect block
	if (blk->block_idx == 13)
	{
		u32_t p = 0;
		lseek(fs->fd, inode.i_block[blk->block_idx] * fs->block_size + blk->block_offset, SEEK_SET);
		read(fs->fd, &p, sizeof(u32_t));

		lseek(fs->fd, p * fs->block_size + blk->d_block_offset, SEEK_SET);
		p = 0;
		blk->d_block_offset += read(fs->fd, &p, sizeof(u32_t));

		if (blk->d_block_offset >= fs->block_size)
		{
			blk->d_block_offset = 0;
			blk->block_offset += sizeof(u32_t);
		}

		if (blk->block_offset >= fs->block_size)
		{
			blk->block_offset = 0;
			blk->block_idx++;
		}

		return p;
	}
	// handle triple indirect block
	if (blk->block_idx == 14)
	{
		u32_t level1_blk = 0;
		u32_t level2_blk = 0;
		u32_t data_blk = 0;
		if (pread(fs->fd, &level1_blk, sizeof(u32_t),
				  blk->inode.i_block[14] * fs->block_size + blk->block_offset) != sizeof(u32_t))
			return 0;

		if (level1_blk == 0)
			return 0;

		if (pread(fs->fd, &level2_blk, sizeof(u32_t),
				  level1_blk * fs->block_size + blk->d_block_offset) != sizeof(u32_t))
			return 0;

		if (level2_blk == 0)
			return 0;

		if (pread(fs->fd, &data_blk, sizeof(u32_t),
				  level2_blk * fs->block_size + blk->dd_block_offset) != sizeof(u32_t))
			return 0;

		blk->dd_block_offset += sizeof(u32_t);
		if (blk->dd_block_offset >= fs->block_size)
		{
			blk->dd_block_offset = 0;
			blk->d_block_offset += sizeof(u32_t);
		}

		if (blk->d_block_offset >= fs->block_size)
		{
			blk->d_block_offset = 0;
			blk->block_offset += sizeof(u32_t);
		}

		if (blk->block_offset >= fs->block_size)
		{
			blk->block_idx++;
		}

		return data_blk;
	}

	return 0;
}

void ext2_fs_free(ext2_fs fs)
{
	memhelper_free((void **)&fs->super);
	memhelper_free((void **)&fs->group_desc);
}
