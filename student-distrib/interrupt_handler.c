#include "interrupt_handler.h"
#include "x86_desc.h"
#include "lib.h"
#include "syscalls.h"
#include "exception_handler.h"
#include "keyboard.h"
#include "rtc.h"
#include "interrupt_linkage.h"
#include "syscall_linkage.h"
#include "schedule.h"

/* init_idt()
 * Description: Initialize IDT, fill each entry with base values
 * Inputs: none
 * Outputs: none
 * Returns: none
 * Side Effects: None
 */
void init_idt() {
    unsigned i = 0; /* Counter */

    /* initializing exceptions */
    for(i = 0; i < 32; i++) 
    {
        idt[i].seg_selector = KERNEL_CS; /* idt is operating in kernel space */
        idt[i].reserved4 = 0; /* intel defined reserved bits, Chapter 5.11 */
        idt[i].reserved3 = 1; /* reserved 3 is 1 for exceptions (uses TRAP gate) */
        idt[i].reserved2 = 1;
        idt[i].reserved1 = 1; 
        idt[i].size = 1;      /* 32-bit interrupt registers? 1 is 32 bit, 0 is 16 bit */
        idt[i].reserved0 = 0;
        idt[i].dpl = 0;       /* DPL is 0 for exceptions          */
                              /* DPL = Descriptor privilege level */
        idt[i].present = 1;
    }

    /* initializing interrupts */
    for(i = 32; i < 256; i++) 
    {
        idt[i].seg_selector = KERNEL_CS; /* idt is operating in kernel space */
        idt[i].reserved4 = 0; /* intel defined reserved bits, Chapter 5.11 */
        idt[i].reserved3 = 0; /* reserved 3 is 0 for interrupts (uses interrupt gate) */
        idt[i].reserved2 = 1;
        idt[i].reserved1 = 1; 
        idt[i].size = 1;      /* 32-bit interrupt registers? 1 is 32 bit, 0 is 16 bit */
        idt[i].reserved0 = 0;
        idt[i].dpl = 0;       /* DPL is 0 for interrupts */
        idt[i].present = 1; 
    }

    /* initializing system calls */ 
    idt[0x80].dpl = 3;        /* DPL is 3 for system calls - lower priority */
    idt[0x80].reserved3 = 1;  /* Reserved 3 is 1 for system calls (uses interrupt gate) */

    /* Setting the IDT offsets 
     * IDT entries 9, 15, & 20-31 are reserved */
    SET_IDT_ENTRY(idt[0], exception_DE);
    SET_IDT_ENTRY(idt[1], exception_DB);
    SET_IDT_ENTRY(idt[2], exception_NMI); 
    SET_IDT_ENTRY(idt[3], exception_BP);
    SET_IDT_ENTRY(idt[4], exception_OF);
    SET_IDT_ENTRY(idt[5], exception_BR);
    SET_IDT_ENTRY(idt[6], exception_UD);
    SET_IDT_ENTRY(idt[7], exception_NM);
    SET_IDT_ENTRY(idt[8], exception_DF);
    SET_IDT_ENTRY(idt[9], exception_CS);
    SET_IDT_ENTRY(idt[10], exception_TS);
    SET_IDT_ENTRY(idt[11], exception_NP);
    SET_IDT_ENTRY(idt[12], exception_SS);
    SET_IDT_ENTRY(idt[13], exception_GP);
    SET_IDT_ENTRY(idt[14], exception_PF);
    SET_IDT_ENTRY(idt[16], exception_MF);
    SET_IDT_ENTRY(idt[17], exception_AC);
    SET_IDT_ENTRY(idt[18], exception_MC);
    SET_IDT_ENTRY(idt[19], exception_XF);

    SET_IDT_ENTRY(idt[0x20], PIT);      /* Set IDT offset for PIT interrupts      */
    idt[0x20].present = 1;              /* PIT interrupts marked as present       */

    SET_IDT_ENTRY(idt[0x21], keyboard); /* Set IDT offset for keyboard interrupts */
    idt[0x21].present = 1;              /* Keyboard interrupts marked as present  */

    SET_IDT_ENTRY(idt[0x28], RTC);      /* Set IDT offset for RTC interrupts      */
    idt[0x28].present = 1;              /* RTC interrupts marked as present       */

    SET_IDT_ENTRY(idt[0x80], sys_call); /* Set IDT offset for System Calls        */
    idt[0x80].present = 1;              /* System Calls marked as present         */
}
