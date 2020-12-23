#include "schedule.h"
#include "lib.h"
#include "i8259.h"
#include "syscalls.h"
#include "x86_desc.h"
#include "page.h"
#include "keyboard.h"

/* low/byte mode values for initializing the pit to 10 ms */
static uint8_t low =  (NATURAL_FREQ / DESIRED_FREQ) & LOWER_MASK;
static uint8_t high =  ((NATURAL_FREQ / DESIRED_FREQ) & UPPER_MASK) >> 8; /* Shift 8 to get it into lower 8 bits */

/* init_PIT()
 * Description: Initialize the PIT by setting the mode and intializing the counter vals for PIT to interrput at 10 ms intervals .
 *              Inspired by OSDev.
 * Inputs: none
 * Outputs: none
 * Returns: none
 * Side Effects: Modifies global variables: total_processes, total_base, current_process, pid. 
 */
void init_PIT() {
    total_processes = 0;                      /* there are no intial processes                                  */
    total_base = 0;                           /* there are no intial base shells                                */
    current_process = &processes[0];          /* the first process is the first element in global process array */
    current_process->next = current_process;  /* create the circular linked list                                */
    pid = 0;                                  /* the first pid will be 0                                        */
    outb(PIT_ICW0, PIT_CMD_PORT);             /* init the PIT with the command words and counter values         */
    outb(low, PIT_DATA_0);
    outb(high, PIT_DATA_0);
    enable_irq(PIT_IRQ);                      /* notify PIC to allow PIT to interrupt                           */
}

/* pit_handler()
 * Description: For the first 3 interrupts, it starts 3 base shell programs. Otherwise, it performs a context switch 
 *              to the next scheduled process
 * Inputs: none
 * Outputs: none
 * Returns: none
 * Side Effects: calls context_switch which changes many global vars and changes the stack frame
 */
void pit_handler() {
    send_eoi(PIT_IRQ);
    /* start 3 base shells */
    switch(total_processes) {
        case 0:
            start_terminal(TERM0);
            return;
        case 1:
            swap_terminals(TERM1);
            return; 
        case 2:
            swap_terminals(TERM2);
            return; 
        default:
            break;
    }
    /* always perform a context switch to next process after the base shells have started */
    context_switch(current_process->next); 
}

/* context_switch()
 * Description: Called by the pit_handler. Performs a context switch to the next scheduled program
 * Inputs: none
 * Outputs: none
 * Returns: none
 * Side Effects: Stack frame (esp/ebp) is updated, current_process is current_process->next, tss is updated with next_process->esp0 and KERNEL_DS
 *               virtual memory is remapped, calls start_process which updates other global vars.
 */
void context_switch(process_t * next_process) {
    /* store ebp and esp */
    asm volatile(
        "movl %%esp, %%eax;"
        "movl %%ebp, %%ebx;"
        :"=a"(current_process->esp), "=b"(current_process->ebp)
    );    
    /* Make sure the displayed terminal is valid, otherwise start a shell there. */
    if(terminals[curr_tid].active == NULL) {
        execute((const uint8_t *) "shell");
    }
    else {
        /* remap next program virtual memory */
        vmap(_128MB, next_process->pid);

        /* set ss0 to be KERNEL_DS, and esp0 to point to the new process's kernel stack */
        tss.ss0 = KERNEL_DS;
        tss.esp0 = next_process->esp0;

        /* start up the process */
        start_process(next_process);

        /* update stack frame to switch to next scheduled process */
        asm volatile(
            "movl %0, %%esp;"
            "movl %1, %%ebp;"
            :
            : "r"(next_process->esp), "r"(next_process->ebp)
        );
    }
    return;
}

/* add_process()
 * Description: Adds a process to the scheduler linked list and global processes array.
 * Inputs: process_t *p - pointer to a process struct to be added to linked list and global processes array
 * Outputs: none
 * Returns: 0 if added successfully, -1 if failed 
 * Side Effects: none
 */
int add_process(process_t* p) {
    if (p == NULL) {
        return -1;
    } 
    /* add the process to global struct */
    memcpy(&(processes[p->pid]), p, sizeof(process_t));
    cli();
    process_t *prev = current_process; 
    process_t *temp = current_process->next;
    /* find its parent in the linked list */
    while(temp != p->parent) {
        /* Handles case where if there's no parent in the list, it just adds it to the linked list */
         if(temp == current_process) {
             prev->next = &(processes[p->pid]);
            (&(processes[p->pid]))->next = temp;
            return 0;
        }
        prev = prev->next;
        temp = temp->next;
    }
    /* re-adjust pointers in linked list */
    process_t * temp_next = temp->next;
    prev->next = &(processes[p->pid]);
    (&(processes[p->pid]))->next = temp_next;
    sti();
	return 0;
}

/* remove_process()
 * Description: Removes a process from scheduler linked list and global processes array. Done by replacing it with its parent process
 * Inputs: process_t *p - pointer to a process struct to be removed from linked list and global processes array
 * Outputs: none
 * Returns: 0 if removes successfully, -1 if failed
 * Side Effects: current_process is updated 
 */
int remove_process(process_t * p) {
    if (p == NULL) {
        return -1;
    }
    process_t *prev = p;
    process_t *temp = p->next; 
    /* Loop to find the process and its prev in the linked list */
    while(temp != p) {
        prev = temp; 
        temp = temp->next; 
    }
    /* Delete the process from the linked list (replace it with its parent) */
    prev->next = &(processes[p->parent->pid]);
    (&(processes[p->parent->pid]))->next = temp->next;
    /* Update the active process inside the terminal struct */
    p->terminal->active = &(processes[p->parent->pid]);
    /* update the current process to be the parent of the deleted process */
    current_process = &(processes[p->parent->pid]);
    return 0; 
}

/* start_process()
 * Description: updates global variables to simulate scheduled process starting up, called by context_switch
 * Inputs: process_t *p - pointer to a process struct to start
 * Outputs: none
 * Returns: 0 if process started successfully, -1 if failed
 * Side Effects: current_process is updated, TLB is refreshed, paging is updated, and global vars (video_mem, current_process, and pid are updated)
 */
int start_process(process_t* p) {
    if (p == NULL) {
        return -1; 
    }
    /* changes global vars based on scheduled process */
	video_mem = p->terminal->vmem;
	current_process = &(processes[p->pid]);
	pid = p->pid;
    p->terminal->active = current_process;

    /* update the paging for the current process */
    Page_Directory[USR_VIDEO_PDE] = (uint32_t)Page_Table | PRESENT | R_W | User_SUP;
    /* If the current process is the displayed terminals, map to physical video mem */
    if(current_process && current_process->terminal == &(terminals[curr_tid])) {
        Page_Table[VIDEO_PTE] = (uint32_t) VIDEO_MEMORY_START | PRESENT | R_W | User_SUP;
    }
    else {
        /* otherwise, map to the background buffer of the current process's video mem */
        Page_Table[VIDEO_PTE] = (uint32_t) video_mem | PRESENT | R_W | User_SUP;
    }
    flush_TLB();
	return 0;
}
