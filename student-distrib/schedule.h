#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "terminal.h"
#include "types.h"
#include "syscalls.h"

#define PIT_IRQ 0
#define PIT_CMD_PORT 0x43
#define PIT_DATA_0 0x40
#define PIT_ICW0 0x36
#define DESIRED_FREQ 100
#define NATURAL_FREQ 1193182
#define MAX_PROCESSES 6
#define LOWER_MASK 0x00FF
#define UPPER_MASK 0xFF00

typedef struct process_t {
	struct terminal_t * terminal; /* Every process is tied to a terminal, allows for lib.c/keyboard.c/terminal.c to work properly */
	uint8_t pid;				  
	PCB * pcb; 
	uint32_t esp; 				  /* Need for context switch */
	uint32_t ebp; 				  /* Need for context switch */
	struct process_t *next; 	  /* scheduler is round-robin (circular) linked list, so we need a next pointer for the next process in queue */
	struct process_t *parent;     /* Once a process finishes, it needs to return to its parent, so we store the parent as well */
	uint32_t esp0; 				  /* need for context switch (updates the tss) */
} process_t;

process_t * current_process;        /* Global Current Process (head of linked list) */
process_t processes[MAX_PROCESSES]; /* Stores the process structs (data container - no functionality)*/

/* Scheduling Functions */
int add_process(process_t* p);
int remove_process(process_t *p);
int start_process(process_t* p);
void context_switch(process_t * next_process); 

/* PIT functions */
extern void init_PIT();
extern void pit_handler();

#endif
