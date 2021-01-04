#include "../lib.h"
#include "i8259.h"
#include "../schedule.h"
#include "../terminal.h"
#include "PIT.h"
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
    // switch(total_processes) {
    //     case 0:
    //         start_terminal(TERM0);
    //         return;
    //     case 1:
    //         swap_terminals(TERM1);
    //         return; 
    //     case 2:
    //         swap_terminals(TERM2);
    //         return; 
    //     default:
    //         break;
    // }
    // /* always perform a context switch to next process after the base shells have started */
    // context_switch(current_process->next); 
}
