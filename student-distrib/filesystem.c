#include "filesystem.h"
#include "lib.h"
//#include "syscalls.h"

static bootblock * boot_block;  /* points to the bootblock in memory     */
static datablock * data_blocks; /* points to first data block in memory  */
static int count = 0;           /* used to make sure successive calls to read_directory returns 0 once it exceeds num of files */

/* init_fs()
 * Description: Initialize filesystem and adjust pointers to memory.
 * Inputs: none
 * Outputs: none
 * Returns: none
 * Side Effects: None
 */
void init_fs() {
    /* module loaded in kernel is the fs, so mod_start is the start address of the fs */
    boot_block = (bootblock * )FS_START->mod_start; 
    /* inodes are 1 block after boot block, so boot_block + 1 points to first inode */
    inodes = (inode *) boot_block + 1; 
    /* datablocks are after number of inodes, + 1 to skip boot_block */
    data_blocks = (datablock * ) FS_START->mod_start + (1 + boot_block->num_inodes); 
}

/* fs_open()
 * Description: The call should find the diretory entry corresponding to the
 *              named file, alloate an unused file desriptor, and set up any data necessary 
 *              to handle the given type of file.
 * Inputs: filename - file name
 * Outputs: none
 * Returns: -1 on failure, 0 otherwise
 * Side Effects: None
 */
int32_t fs_open(const uint8_t * filename) {
    return 0;
}

/* dir_open()
 * Description: Opens the input directory
 * Inputs: filename - file name
 * Outputs: none
 * Returns: 0
 * Side Effects: Resets count 0
 */
int32_t dir_open(const uint8_t * filename) {
    count = 0;
    return 0;
}

/* dir_close()
 * Description: Does nothing
 * Inputs: fd - file directory
 * Outputs: none
 * Returns: 0
 * Side Effects: none
 */
int32_t dir_close(int32_t fd) {
    return 0;
}

/* fs_close()
 * Description: The call closes the specified file desriptor and makes 
 *              it available for return from later calls to open.
 * Inputs: fd - file directory
 * Outputs: none
 * Returns: -1 on failure, 0 otherwise
 * Side Effects: None
 */
int32_t fs_close(int32_t fd) {
    return 0;
}

/* fs_write()
 * Description: Does nothing as filesystem is currently read-only.
 * Inputs: fd - file directory
 *         buf - data to be written to file
 *         nbytes - number of bytes of data to be written
 * Outputs: none
 * Returns: -1 on failure, 0 otherwise
 * Side Effects: None
 */
int32_t fs_write(int32_t fd, const void * buf, int32_t nbytes) {
    return -1;
}

/* dir_write()
 * Description: Does nothing as filesystem is currently read-only.
 * Inputs: fd - file directory
 *         buf - data to be written to directory
 *         nbytes - number of bytes of data to be written
 * Outputs: none
 * Returns: -1 on failure, 0 otherwise
 * Side Effects: none
 */
int32_t dir_write(int32_t fd, const void * buf, int32_t nbytes) {
    return -1;
}

/* read_dentry_by_name()
 * Description: Finds and loads the directory entry for a file corresponding
 *              to the input name.
 * Inputs: fname - string name of file
 *         dentry - empty directory entry struct to fill
 * Outputs: none
 * Returns: -1 on failure, 0 otherwise
 * Side Effects: None
 */
int32_t read_dentry_by_name(const uint8_t *fname, dentry_t * dentry) {
    /* Check to make sure name, dentry pointer aren't null, and name isn't longer than 32 characters */
    if(fname == NULL || dentry == NULL || strlen((const int8_t *) fname) > MAX_FILE_NAME_LEN) {
        return -1;
    }
    int i = 0;
    while(i < boot_block->num_entries) {
        uint32_t length = strlen((const int8_t *) boot_block->entries[i].f_name);
        length = MAX_FILE_NAME_LEN;
        /* use string compare to see if directory name and fname are the same, if so copy the data into user dentry */
        if(strncmp((const int8_t *)fname , (const int8_t *)boot_block->entries[i].f_name, length) == 0) {
            if(strncmp((const int8_t *)fname , (const int8_t *)boot_block->entries[i].f_name, length) == 0) {
                strcpy((int8_t *) dentry->f_name, (const int8_t *) fname);
                dentry->f_name[length] = '\0'; /* terminate the user entry with EOS */
                dentry->f_type = boot_block->entries[i].f_type;
                dentry->inode = boot_block->entries[i].inode;
                return 0;
            }
        }
        i++;
    }
    return -1;
}

/* read_dentry_by_index()
 * Description: Finds and loads the directory entry for a file corresponding
 *              to the input index.
 * Inputs: index - index of file
 *         dentry - empty directory entry struct to fill
 * Outputs: none
 * Returns: -1 on failure, 0 otherwise
 * Side Effects: None
 */
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry) {
    if(index > boot_block->num_entries) { /* ensure index is valid (not greater than num of entries) */
        return -1;
    }
    uint32_t length = strlen((const int8_t *) boot_block->entries[index].f_name);
    /* copy directory entry by accessing boot_block->entries[index] */
    strcpy((int8_t *) dentry->f_name, (const int8_t *)boot_block->entries[index].f_name);
    dentry->f_name[length] = '\0';
    dentry->f_type = boot_block->entries[index].f_type;
    dentry->inode = boot_block->entries[index].inode;
    return 0;
}

/* read_data()
 * Description: Finds and loads the data of a file into a buffer given its inode.
 * Inputs: inode_idx - index of inode corresponding to desired file
 *         offset - offset into file in bytes
 *         buf - buffer to fill with file data
 *         read_length - number of bytes to read
 * Outputs: none
 * Returns: -1 on failure, 0 otherwise
 * Side Effects: None
 */
int32_t read_data (uint32_t inode_idx, uint32_t offset, uint8_t* buf, uint32_t read_length) {
    /* ensure user buffer is valid and inode_idx is valid */
    if(inode_idx > boot_block->num_inodes || buf == NULL) {
        return -1;
    }
    /* use inode_idx to offset into inodes array */
    inode * inode_block = inodes + inode_idx;
    uint32_t file_length = inode_block->length;

    /* offset into the file cannot be greater than the file length itself */
    if (offset >= file_length) {
        return -1; 
    }

    /* we should only read until the end of the file, so adjust read_length if offset+read_length > length of file */
    if (offset + read_length > file_length) {
        /* chop off the part that goes beyond the file length */
        read_length = file_length - offset; 
    }

    uint32_t block = offset / BLOCK_SIZE;    /* flooring offset / BLOCK_SIZE gives which block we want to start reading in*/
    uint32_t data_idx = offset % BLOCK_SIZE; /* the remaining of ^ is how far into the block we want to start reading (so the data) */
    uint32_t buff_idx = 0;                   /* fill user buffer starting from 0 */
    
    while(read_length) {
        buf[buff_idx] = data_blocks[inode_block->data_block[block]].data[data_idx];
        read_length --;
        data_idx    ++;
        buff_idx    ++;
        /* once data_idx goes exceeds the size of the block, reset it 0 and go to next block */
        if(data_idx >= BLOCK_SIZE) {
            block++;
            data_idx = 0;
        }
    }
    /* buff idx is how many bytes were copied at this point */
    return buff_idx; 
}

/* read_directory()
 * Description: Loads the name of a file given its directory into the input buffer.
 * Inputs: fd - file directory who's name is being loaded
 *         buf - buffer to fill with name
 *         nbytes - number of characters to copy
 * Outputs: none
 * Returns: number of bytes copied, 0 if the number of calls exceeds number of files.
 * Side Effects: None
 */
int32_t dir_read(int32_t fd, void * buf, int32_t nbytes) {
    if(buf == 0 || count >= boot_block->num_entries) {
        return 0;
    }
    int bytes_written = nbytes;
    /* ensure that bytes written into buffer does not exceed file name length */
    if(nbytes > MAX_FILE_NAME_LEN) {
        bytes_written = MAX_FILE_NAME_LEN;
    }
    char * buffer = (char * ) buf;
    int i;
    /* Copy name into input buffer */
    for(i = 0; i < bytes_written && boot_block->entries[count].f_name[i] != '\0'; i++) {
        buffer[i] =  (boot_block->entries[count].f_name[i]);
    }
    /* update directory position */
    count++; 
    return i;
}
