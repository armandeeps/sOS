#include "exception_handler.h"
#include "lib.h"
#include "syscalls.h"

/* exception_XX()
 * Description: Prints the name of the exception and BSODs.
 * Inputs: none
 * Outputs: none
 * Returns: none
 * Side Effects: None
 */
void exception_DE() {
    cli();                                 /* Clear Interrupts */
    printf("Division Exception!");         /* Print exception  */ 
    exec_halt(EXCEPTION_RET);              /* Sqaush and halt  */
}

void exception_DB() {
    cli();
    printf("Debug Exception!\n");
    exec_halt(EXCEPTION_RET);
   //while(1); 
}

void exception_NMI() {
    cli();
    printf("Nonmaskable Interrupt Exception!\n");
    exec_halt(EXCEPTION_RET);
  // while(1); 
}

void exception_BP() {
    cli();
    printf("Breakpoint Exception!\n");
    exec_halt(EXCEPTION_RET);
   // while(1); 
}

void exception_OF() {
    cli();
    printf("Overflow Exception!\n");
    exec_halt(EXCEPTION_RET);
    //while(1); 
}

void exception_BR() {
    cli();
    printf("Bound range exceeded!\n");
    exec_halt(EXCEPTION_RET);
    
    //while(1); 
}

void exception_UD() {
    cli();
    printf("Invalid opcode exception!\n");
    exec_halt(EXCEPTION_RET);
    //while(1); 
}

void exception_NM() {
    cli();
    printf("Device not available!\n");
    exec_halt(EXCEPTION_RET);
    //while(1); 
}

void exception_DF() {
    cli();
    printf("Double Fault!\n");
    exec_halt(EXCEPTION_RET);
    //while(1); 
}

void exception_CS() {
    cli();
    printf("Coprocessor Segment Overrun!\n");
    exec_halt(EXCEPTION_RET);
   // while(1); 
}

void exception_TS() {
    cli();
    printf("Invalid TSS Exception!\n");
    exec_halt(EXCEPTION_RET);
   // while(1); 
}

void exception_NP() {
    cli();
    printf("Segment not present!\n");
    exec_halt(EXCEPTION_RET);
   // while(1); 
}

void exception_SS() {
    cli();
    printf("Stack-Segment Fault!\n");
    exec_halt(EXCEPTION_RET);
    // while(1); 
}

void exception_GP() {
    cli();
    printf("General Protection Exception!\n");
    exec_halt(EXCEPTION_RET);
   // while(1); 
}

void exception_PF() {
    cli();
    printf("Page Fault!\n");
    exec_halt(EXCEPTION_RET);
    // while(1);
}

void exception_MF() {
    cli();
    printf("FPU Floating Point Error!\n");
    exec_halt(EXCEPTION_RET);
    // while(1); 
}

void exception_AC() {
    cli();
    printf("Alignment Check Exception!\n");
    exec_halt(EXCEPTION_RET);
    // while(1); 
}

void exception_MC() {
    cli();
    printf("Machine Check Exception!\n");
    exec_halt(EXCEPTION_RET);
    // while(1); 
}

void exception_XF() {
    cli();
    printf("SMID Floating-Point Exception!\n");
    exec_halt(EXCEPTION_RET);
    // while(1);  
}
