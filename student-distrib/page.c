#include "page.h"

/* init_paging()
 * Description: Initialize and enable paging by filling the
 *              page table array (for first 4 MB).  Bits values
 *              used are found in descriptors pdf.
 * Inputs: none
 * Outputs: none
 * Returns: none
 * Side Effects: Modify CR0, CR3, & CR4 Registers.
 */
void init_paging(){
    int i; /* Counter */
    uint32_t data; 

    for(i = 0; i < PDE_SIZE; i++) {
        data = 0;  /* Set all bits to 0 */

        /* First page directory corresponds to first 4 MB. 
         * First page directory has 4KB sized pages     */
        if (i == 0) {
            /* Put 20 MSBs of page table address into directory entry */
            data = ((uint32_t)Page_Table >> 12) << 12;  
            data |= PRESENT; /* Mark entry as present */
        }

        /* Second Page directory corresponds to second 4 MB
         * Second directory contains one 4MB page (kernel)  */
        if (i == 1) {
            data = KERNEL_ADDR; /* All entries offset to kernel              */
            data |= PRESENT;    /* Mark entry as present                     */
            data |= PS;         /* Set page size to 4 MB                     */
            data |= GLOBAL;     /* Kernel should be loaded for every program */
        }

        data |= R_W;  /* All entries are set to read/write mode */
        Page_Directory[i] = data;  /* Fill entry with data      */
        Page_Table[i] = FOURKB*i | R_W;  /* Fill other page table entries with address, but not present */
    }

    /* Video memory page in first page directory must be mapped to video memory */
    Page_Table[VIRTUAL_VIDEO_START] = (VIRTUAL_VIDEO_START << 12) | (PRESENT) | (R_W);

    /* Enable paging */
    asm volatile(
                 "movl $Page_Directory, %%eax;"
                 "movl %%eax, %%cr3;" /* cr3 holds the address of our page directory */
                 "movl %%cr4, %%eax;"
                 "orl $0x00000010, %%eax;" /* bit 5 of cr4 allows 4 mB pages */
                 "movl %%eax, %%cr4;"
                 "movl %%cr0, %%eax;"
                 "orl $0x80000000, %%eax;" /* once bits are set, enable paging (msb of cr0) */
                 "movl %%eax, %%cr0;"
                 :                      /* no outputs */
                 :                      /* no input */
                 :"%eax"                /* clobbered register */
                 );
}

