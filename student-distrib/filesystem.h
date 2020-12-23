#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "multiboot.h"
#include "types.h"

#define MAX_FILE_NAME_LEN 32
#define BLOCK_SIZE 4096
#define METADATA_SIZE 64
#define NUM_DENTRIES 63
#define NUM_DATA_BLOCKS 1023
#define DENTRY_RESERVE 24
#define BOOT_RESERVE 52
typedef struct dentry_t {
	unsigned char f_name[MAX_FILE_NAME_LEN];
	unsigned int f_type;
	unsigned int inode;
	unsigned char reserved[DENTRY_RESERVE]; 
} dentry_t;

typedef struct bootblock {
	unsigned int num_entries;
	unsigned int num_inodes;
	unsigned int num_blocks;
	unsigned char reserved[BOOT_RESERVE];
	dentry_t entries[NUM_DENTRIES];   
} bootblock;

typedef struct inode {
	uint32_t length;
	uint32_t data_block[NUM_DATA_BLOCKS];
} inode;

typedef struct datablock {
	 uint8_t data[BLOCK_SIZE];
} datablock;

module_t * FS_START; /* global pointer to struct of filesystem */
inode * inodes;      /* global pointer to first inode block in memory */

void init_fs();
int32_t fs_open(const uint8_t * filename);
int32_t fs_close(int32_t fd);
int32_t fs_write(int32_t fd, const void * buf, int32_t nbytes);
int32_t read_dentry_by_name (const uint8_t* fname,dentry_t* dentry);
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry);
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);
int32_t dir_read(int32_t fd, void * buf, int32_t nbytes);
int32_t fs_read(int32_t fd, void* buf, int32_t nbytes);
int32_t dir_open(const uint8_t * filename);
int32_t dir_close(int32_t fd);
int32_t dir_write(int32_t fd, const void * buf, int32_t nbytes);

#endif
