#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "types.h"

#define _4GB 4294967296
#define _8MB 8388608
#define _4MB 4194304
#define _128MB 134217728
#define _8KB 8192
#define EXEC_OFFSET 0x48000
#define STD_IN 0
#define STD_OUT 1
#define WRITE 0
#define READ 1
#define OPEN 2
#define CLOSE 3
#define MAX_PROCESSES 6
#define MAX_FILES 8
#define ASCII_NEWLINE 0x0A
#define MAX_FILE_LEN 32 
#define MAX_BUFF_LEN 128
#define NUM_OF_MAGIC_CHARS 4
#define ASCII_DEL 0x7F
#define INSTR_START 24
#define IN_USE 1
#define NOT_IN_USE 0
#define NUM_JMP_TABLES 3
#define NUM_OPS 4
#define START 0
#define MIN_FILES 0
#define EXEC_TYPE 2

typedef struct file_desc_t {
    uint32_t * func_ptr;            /* each file type has a standard interface       */
    uint32_t inode;                 /* index into inode array                        */
    uint32_t file_pos;              /* how "far" into a file the user is             */
    uint32_t flags;                 /* used to mark a fd as "in use" or "not in use" */
} file_desc_t;

typedef struct PCB {
    file_desc_t file_ops[MAX_FILES]; /* stores the file descriptors associated with a process, setup/cleared upon open/close */
	uint32_t parent_esp;             /* ensures we can swap context properly                                                 */
    uint32_t parent_ebp; 
	uint32_t pid;                    /* used to keep track of processes, indexes into processes array and used for pcb address math */
	struct PCB * parent;             /* linked a process with its parent for context swap                                           */
    uint8_t cmd_line[MAX_BUFF_LEN];  /* Processes have different command lines (used for args)                                      */
} PCB;

int pid;             /* Current process id                                           */
int total_processes; /* total active processes                                       */
int total_base;      /* total base shells, used to prevent exiting from a base shell */

/* System calls */
extern int32_t halt(uint8_t status);
int vmap(uint32_t vaddr, uint32_t pid_);
extern int32_t execute(const uint8_t *);
extern int32_t write (int32_t fd, const void* buf, int32_t nbytes);
extern int32_t open (const uint8_t* filename);
extern int32_t getargs (uint8_t* buf, int32_t nbytes);
extern int32_t exec_halt(uint32_t status);
extern int32_t close (int32_t fd);
extern int32_t vidmap (uint8_t** screen_start);

/* System call helpers */
PCB * createPCB();
int8_t get_pid();
extern void parse(const uint8_t * buf, uint8_t * buf2, int i, int length);

#endif 
