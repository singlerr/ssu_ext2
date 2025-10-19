Perfect 💡  
Here’s a professionally formatted **English `README.md` (GitHub style)** for the newly uploaded **“25 Project #3-V0.94.pdf”** — i.e., the **SSU-EXT2** project.  
It’s written for clarity and uses consistent technical documentation style with Markdown syntax (good for actual GitHub use).

---

# 🗂️ SSU-EXT2

## 📘 Overview

**SSU-EXT2** is a Linux system programming project that analyzes an **ext2 filesystem image**, exploring directory and file structures starting from the root directory.  

The goal is to understand the **internal structure and metadata of the ext2 filesystem**, and to implement a shell-based analysis tool that replicates certain filesystem behaviors using low-level system programming techniques.

---

## 🎯 Objectives

- Understand the **design and layout of the ext2 filesystem** (superblock, group descriptor, inode, directory entries, etc.).  
- Create a shell-style program to **parse and interpret binary filesystem data** from an image file.  
- Implement **directory traversal** and **file content analysis** without relying on external ext2 libraries.  
- Build a **linked list-based directory structure** to display hierarchical relationships between files and directories.  
- Enhance system-level programming and filesystem analysis skills.

---

## ⚙️ Program Information

- **Executable name:** `ssu_ext2`  
- **Platform:** Linux  
- **Execution type:** Foreground (background `&` execution not supported)
- **First argument:** ext2 image file path (must exist)
- **Home path:** Derived from the logged-in user (e.g., `/home/oslab/`)
- **No external ext2 libraries:**  
  > You **cannot** use `libext2fs.h`, `ext2_fs.h`, or any related ext2 helper libraries.  
  Instead, you must define any needed structures manually.

🚫 **Do not use:** `system()` — results in 0 points.  
✅ **Recommended:** use standard system calls and `getopt()` for argument parsing.

---

## 💻 Usage

```
Usage: ./ssu_ext2 
```

If no argument is given:
```
./ssu_ext2
Usage Error : ./ssu_ext2 
```

Example:
```
% ./ssu_ext2 ~/ext2disk.img
20250000> 
20250000> exit
%
```

---

## 🧩 Built-in Commands

The program supports four built-in commands:

| Command | Description |
|----------|-------------|
| `tree`   | Display directory hierarchy inside the ext2 image |
| `print`  | Print contents of a file inside the image |
| `help`   | Display command usage information |
| `exit`   | Exit the program |

---

## 🌳 1. Command: `tree`

**Usage:**
```
tree  [OPTIONS...]
```

Display the structure of a directory from the ext2 filesystem image.

### Arguments

| Argument | Description |
|-----------|--------------|
| `` | Directory path (use relative path from the root “.”). |
| `[OPTIONS]` | Optional flags: `-r`, `-s`, `-p`. Can be combined. |

### Output Behavior

- Displays all files and subdirectories under the specified ``.
- Ignores special directories: `"."`, `".."`, and `"lost+found"`.
- Counts and shows total number of **directories** and **files**.

---

### 🔹 Options

| Option | Description | Example Output |
|---------|-------------|----------------|
| `-r` | Recursively prints subdirectory contents | Prints nested structure under each subdir |
| `-s` | Displays file and directory sizes in bytes | `[4096] A` |
| `-p` | Displays permission bits | `[drwxr-xr-x] A` |
| Combined use | All flags can be combined like `-rsp` | `[drwxr-xr-x 4096] A` |

**Example:**
```
20250000> tree . -rsp
[drwxr-xr-x 4096] .
| [drwxr-xr-x 4096] A
| [drwxr-xr-x 4096] B
| [-rw-r--r-- 339] ssu_open.c
↳ [-rw-r--r-- 75] ssu_test.txt
3 directories, 2 files
```

---

### 🧱 Error Handling

| Condition | Message / Behavior |
|------------|--------------------|
| Directory does not exist | Print usage and re-prompt |
| `` is a file, not a directory | `"Error: '' is not directory"` |
| Invalid option input | Print usage and re-prompt |

---

## 📝 2. Command: `print`

**Usage:**
```
print  [OPTION...]
```

Prints the contents of a file located in the ext2 image.

### Arguments

| Argument | Description |
|-----------|--------------|
| `` | Relative path from the image root (e.g., `B/BB/bye.cpp`) |
| `-n ` | (Optional) Print only the first `` lines |

### Examples

```
20250000> print ssu_test.txt
Linux System Programming!
Unix System Programming!
Linux Mania
Unix Mania
```

```
20250000> print ssu_test.txt -n 2
Linux System Programming!
Unix System Programming!
```

**Error Handling**
| Condition | Description |
|------------|-------------|
| Invalid path | Shows usage and re-prompts |
| Directory input | Prints: `"Error: '' is not file"` |
| Missing argument for `-n` | Prints getopt-style error: `option requires an argument -- 'n'` |

---

## 🆘 3. Command: `help`

**Usage:**
```
help [COMMAND]
```

Displays usage descriptions for all built-in commands.  
If a command name is specified, only that command’s help is shown.

**Example:**
```
20250000> help
Usage :
  > tree  [OPTION]...  : display directory structure
      -r : recursive view
      -s : include file sizes
      -p : include permissions
  > print  [OPTION]... : print file contents
      -n  : show only first N lines
  > help [COMMAND]           : show commands
  > exit                     : exit program
```

Invalid command handling:
```
20250000> help xyz
invalid command -- 'xyz'
Usage :
  ...
```

---

## 🚪 4. Command: `exit`

**Usage:**
```
exit
```

Terminates the program.

**Example:**
```
20250000> exit
%
```

---

## 🔍 EXT2 Image Creation Guide

You can create your own ext2 image for testing with the following commands:

```bash
# 1. Create an empty disk image (100MB)
dd if=/dev/zero of=ext2disk.img bs=1M count=100

# 2. Format it as ext2
mkfs.ext2 ext2disk.img

# 3. Mount for file operations
sudo mkdir -p /mnt/ext2disk
sudo mount -o loop ext2disk.img /mnt/ext2disk

# 4. Work on mounted directory
sudo cp file.txt /mnt/ext2disk/
sudo umount /mnt/ext2disk
```

---

## 🧠 Implementation Notes

- You must **analyze binary image data directly** using `open()`, `read()`, and pointer arithmetic.  
- Understand and manually define the following ext2 metadata:
  - **Superblock**
  - **Group Descriptor**
  - **Inode structure**
  - **Directory entries**
  - **Block bitmap / inode bitmap**
- Directory traversal must rely on manually decoded inodes and directory blocks.

---

## 🏗️ Suggested Data Structures

### Example: `struct ext2_super_block`
```c
struct ext2_super_block {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint16_t s_magic; /* EXT2_SUPER_MAGIC = 0xEF53 */
};
```

### Example: `struct ext2_inode`
```c
struct ext2_inode {
    uint16_t i_mode;
    uint32_t i_size;
    uint32_t i_blocks;
    uint32_t i_block[15]; // data block pointers
    uint16_t i_links_count;
};
```

---

## 🧾 Evaluation Criteria

| Component | Description | Max Score |
|------------|-------------|-----------|
| `ssu_ext2` main shell | Core interactive program | 30 |
| Linked List Structure | Used in `tree` or `print` | +10 |
| `tree` command | Directory view | 37 |
|  `-r` | Recursive option | +10 |
|  `-s` | Show size option | +10 |
|  `-p` | Show permission option | +10 |
| `print` command | File output | 30 |
|  `-n` option | Line limit | +5 |
| `help` | Usage summary | 1 |
| `exit` | Program termination | 1 |
| `Makefile` | Correct build process | 1 |
| **Total** |  | **100 points** |

---

## 🧩 Project Policy

- All code must be **original** and run correctly under Linux.
- Document must include:
  1. Detailed overview  
  2. Function list & prototypes  
  3. Flowcharts for each function  
  4. Execution results  
  5. Text-based source with comments
- Avoid binary copies or screenshots of code.

---

## 📘 References

- ext2 header structures (`linux/ext2_fs.h`)
- `fdisk`, `mkfs`, `mount`, `hexdump`, `fsck` commands
- Soongsil University System Programming Course (CSE)

---

## Implementation Hints

- Open and read image
  - Use open(2) with O_RDONLY and pread(2)/lseek+read to access blocks reliably.
  - Always check return values and errno.

- Superblock
  - Superblock starts at byte offset 1024 in the image.
  - Read sizeof(superblock) from offset 1024.
  - Verify magic (EXT2_SUPER_MAGIC, 0xEF53) to ensure a valid ext2 image.
  - Block size = 1024  1024, locate by block boundary).
  - Read necessary fields: bg_block_bitmap, bg_inode_bitmap, bg_inode_table.

- Inode table and locating an inode
  - Inode size typically in superblock (s_inode_size). If not, assume 128 or 256 as specified.
  - Inode N offset = inode_table_block * block_size + (N - 1) * inode_size.
  - Read inode structure and interpret fields (i_mode, i_size, i_block[], i_links_count, i_blocks).

- Directory entries
  - Directory data blocks contain variable-length directory entries:
    - struct dir_entry { uint32 inode; uint16 rec_len; uint8 name_len; uint8 file_type; char name[]; }
  - Walk each directory block reading rec_len to jump to the next entry.
  - Ignore entries with inode == 0.
  - File type values: 1=regular, 2=directory, 7=symlink, etc.
  - Skip "." and ".." as needed for presentation rules.

- Reading file data
  - Use inode i_block[] direct pointers (first 12 entries are direct).
  - For single-indirect, double-indirect, triple-indirect, follow block pointers accordingly (implement at least single-indirect; higher levels if required by tests).
  - Compute file block offset = block_number * block_size; read blocks sequentially until file length consumed.

- Permissions and mode parsing
  - i_mode lower bits indicate file type and permission bits. Use macros:
    - S_IFDIR, S_IFREG, and permission bit masks (owner/group/other read/write/exec).
  - Format permission string like ls (e.g., drwxr-xr-x).

- Size and statistics
  - For directories, you may display the directory’s i_size or block allocation size as needed by `-s`.
  - Maintain counters for number of directories and files printed.

- Path resolution and traversal
  - Implement a function to resolve relative paths from image root:
    - Start from root inode (inode 2).
    - For each path component, scan the directory entries to find matching name -> get its inode.
  - Implement recursion for `-r` option with depth control to avoid infinite loops (detect cyclic links).

- Linked list structure
  - Represent directory nodes and file nodes as structs in a linked list (or tree) for `tree` output and counting:
    - struct node { char *name; int is_dir; uint32 inode; vector/list children; ... }
  - Populate nodes on demand (lazy) or build full tree before printing.

- I/O and buffering
  - Use buffered reads for block-sized operations.
  - Avoid using system() or external libraries for parsing.

- Error handling
  - Print usage messages when invalid arguments/options provided.
  - Validate path existence and type (dir vs file) and present clear error messages.

- Testing tip
  - Create an ext2 image:
    - dd if=/dev/zero of=ext2disk.img bs=1M count=100
    - mkfs.ext2 ext2disk.img
    - mkdir -p /mnt/ext2 && sudo mount -o loop ext2disk.img /mnt/ext2
    - Populate files and directories, then sudo umount /mnt/ext2
  - Use hexdump -C to inspect block offsets and verify pointer arithmetic.

- Useful syscalls & libc functions
  - open, close, pread, read, lseek, malloc/free, memcpy, fprintf, snprintf
  - getopt for option parsing
  - time/strftime if you need human-readable timestamps (optional for logging)

- Endianness
  - ext2 structures are stored in little-endian. On little-endian x86/x86_64 you can read directly; be careful on other architectures.
  - Use explicit conversions (e.g., le32toh) if portability is required.
