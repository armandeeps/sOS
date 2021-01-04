#include "schedule.h"
#include "page.h"
#include "lib.h"
#include "devices/keyboard.h"
#include "interrupts/syscalls.h"
#include "devices/rtc.h"

static uint8_t colors[MAX_TERMINALS] = {WHITE, CYAN, GREEN};

/* init_terminals()
 * Description: Fills the global terminal array upon bootup 
 * Inputs: process_t none
 * Outputs: none
 * Returns: none
 * Side Effects: sets the global video_mem to be the displayed video_mem, and global curr_tid to be 0, paging is updated
 */
void init_terminals() {
    video_mem = (uint8_t *)VIDEO;
    int i, j;
    for(i = 0; i < MAX_TERMINALS; i++) {
        /* Fill the struct with initial values */
        terminals[i].tid = i;
        terminals[i].curr_x = 0;
        terminals[i].curr_y = 0;
        terminals[i].buff_idx = 0;
        terminals[i].vmem = create_vmem(i);
        terminals[i].color = colors[i];
        terminals[i].active = NULL;
        terminals[i].term_freq = HARDWARE_FREQ;
        terminals[i].term_rtc_counter = HARDWARE_FREQ / terminals[i].term_freq;
        terminals[i].term_rtc_flag = SET; 
        asm volatile(
        "pushfl;"
        "popl %0;"
        :"=r"(terminals[i].flags)
        );
        /* Fill terminal's video buffer with NULL's and attribute colors */
        for(j = 0; j < NUM_ROWS*NUM_COLS; j++) {
            terminals[i].vmem[j << 1] = ' ';
            terminals[i].vmem[(j << 1) + 1] = terminals[i].color;
        }
        for(j = 0; j < BUFF_SIZE; j++) {
            terminals[i].buff[j] = '\0';
        }
    }
    curr_tid = START;
}

/* start_terminals()
 * Description: Performs necessary changes to global variables to switch to a displayed terminal.  
 * Inputs: tid - id of the terminal the user wants to switch to
 * Outputs: none
 * Returns: 0 if success, -1 if failed 
 * Side Effects: Paging is updated, context_switch is called 3 times upon bootup
 */
int start_terminal(uint8_t tid) {
    /* Make sure tid is valid */
    if(tid >= MAX_TERMINALS) {
        return -1;
    }

    /* Update variables for other terminal, keyboard, and lib.c functions to work properly */
    curr_tid = tid;
    kb_buff = terminals[tid].buff;
    buff_idx = terminals[tid].buff_idx;
    ATTRIB = terminals[tid].color;

    /* Copy the video memory of buffer we're switching into to the visible video memory location */
    memcpy((uint8_t *)VIDEO_MEMORY_START, (const uint8_t *)terminals[tid].vmem, _4KB);
    
    /* Update the paging and flush the TLB afterwards */
    Page_Directory[USR_VIDEO_PDE] = (uint32_t)Page_Table | PRESENT | R_W | User_SUP;
    /* If the current process is the displayed terminals, map to physical video mem */
    if(current_process && current_process->terminal == &(terminals[curr_tid])) {
        Page_Table[VIDEO_PTE] = (uint32_t) VIDEO_MEMORY_START | PRESENT | R_W | User_SUP;
    }
    else {
        /* otherwise, map to the background buffer of the current process's video mem */
        Page_Table[VIDEO_PTE] = (uint32_t) video_mem | PRESENT | R_W | User_SUP;
    }
    /* SHIFT 12 to get top 20 MSB */
    Page_Table[(uint32_t) terminals[tid].vmem>>12] = VIDEO_MEMORY_START | PRESENT | R_W;
    flush_TLB();

    /* Update the screen coordinates */
    set_screen_coordinates(terminals[tid].curr_x, terminals[tid].curr_y);
    asm volatile(
        "pushl %0;"
        "popfl;"
        :
        :"r"(terminals[tid].flags)
    );
    // if(total_processes < MAX_TERMINALS){
        context_switch(terminals[tid].active);
    // }
    return 0;
}

/* create_vmem()
 * Description: Creates a virtual video mem buffer so that processes can can write to video memory properly (in focus or in the background) 
 * Inputs: tid - id of the terminal a portion of video mem will be tied to
 * Outputs: none
 * Returns: pointer to video memory buffer if successful, otherwise returns NULL
 * Side Effects: Paging is updated
 */
uint8_t * create_vmem(uint8_t tid) {
    if (tid >= MAX_TERMINALS) {
        return NULL; 
    }
    uint32_t vaddr = VIDEO_MEMORY_START + (1 + tid)*_4KB;
    /* SHIFT 12 to get top 20 MSB */
    Page_Table[vaddr>>12] = vaddr | (PRESENT) | (R_W) | User_SUP;
    flush_TLB();
    return (uint8_t*) vaddr;
}

/* save_terminal_state()
 * Description: Copies the current terminal state and flags into the global terminal array 
 * Inputs: none
 * Outputs: none
 * Returns: pointer to video memory buffer if successful, otherwise returns NULL
 * Side Effects: Paging is updated
 */
void save_terminal_state() {
    /* Save relevant variables for terminal restoration */
    terminals[curr_tid].buff_idx = buff_idx;
    terminals[curr_tid].curr_x = get_screen_x();
    terminals[curr_tid].curr_y = get_screen_y();

    uint32_t vaddr = (uint32_t) terminals[curr_tid].vmem;
    /* SHIFT 12 to get top 20 MSB */
    Page_Table[vaddr>>12] = vaddr | (PRESENT) | (R_W);
    flush_TLB();
     asm volatile(
        "pushfl;"
        "popl %0;"
        :"=r"(terminals[curr_tid].flags)
    );
    /* Copy current displayed video_mem into terminal's memory buffer */
    memcpy((uint8_t *)vaddr, (const uint8_t*)VIDEO_MEMORY_START, _4KB);
}

/* swap_terminals()
 * Description: Saves the terminal state and starts the terminal user wants to switch to
 * Inputs: tid - terminal id that user wants to switch into 
 * Outputs: none
 * Returns: 0 if successful, -1 if failed
 * Side Effects: Calls save_terminal_state and start_terminal
 */
int swap_terminals(uint8_t tid) {
    if(tid >= MAX_TERMINALS) {
        return -1;
    }
    /* perform the swap */
    save_terminal_state();
    start_terminal(tid);
    return 0;
}

/* terminal_read()
 * Description: Wait for user input into keyboard buffer and copy the keyboard 
 *              buffer into the input buffer. 
 * Inputs: ignore - fd in future
 *         buffer - buffer to fill with keyboard buffer
 *         n - number of bytes to copy into keyboard buffer
 * Outputs: none
 * Returns: number of bytes successfully copied into buffer
 * Side Effects: None
 */
int32_t terminal_read(uint32_t ignore, void * buffer, uint32_t n) {
    /* Ensure passed in buffer is valid */
    if (buffer == NULL) {
        return 0; 
    }
    /* Read the current active process's buffer */
    volatile uint8_t * tb = terminals[current_process->terminal->tid].buff;
    buff_idx = terminals[current_process->terminal->tid].buff_idx;

    /* wait until the active process's buffer is terminated with a newline */
    while(tb[buff_idx]!='\n');

    int i, j;
    clear_buffer((unsigned char*) buffer, n);
    unsigned char * buffer_copy = (unsigned char *) buffer;
    /* copy into user buffer, ensure less than buffer size or n, whichever is smaller */
    for(i = 0; i < BUFF_SIZE && i < n; i++) { 
        buffer_copy[i] = tb[i];
        if(tb[i] == '\n'){
            j = i;
        }
    }

    /* clear the active process's kb_buff before returning, and reset the buff_idx */
    clear_buffer((unsigned char*)tb, BUFF_SIZE); 
    terminals[current_process->terminal->tid].buff_idx = 0; 
    /* j + 1 is how many bytes were copied at this point */
    return j+1; 
}

/* terminal_write()
 * Description: Write the contents of a buffer to the terminal.
 * Inputs: buffer - buffer to print to terminal
 *         n - number of chars to display
 * Outputs: none
 * Returns: number of bytes successfully copied into buffer
 * Side Effects: None
 */
int32_t terminal_write(uint32_t ignore, void * buffer, uint32_t n) {
    /* Ensure passed in buffer is valid */
    if(buffer == NULL) {
        return 0;
    }
    uint8_t *buff = (uint8_t*) buffer;
    int i;
    /* write a user defined buffer onto screen */
    for(i = 0; i < n; i++) { 
        if (buff[i] != NULL) {
            putc(buff[i]);
        }
    }
    return i;
}

/* terminal_open()
 * Description: Set up data necessary for terminal to be operational. Not
 *              implemented yet.
 * Inputs: filename
 * Outputs: none
 * Returns: 0
 * Side Effects: None
 */
int32_t terminal_open(const uint8_t * filename) {
    return 0;
}

/* terminal_close()
 * Description: Tear down data necessary for terminal operations. Not
 *              implemented yet.
 * Inputs: filename
 * Outputs: none
 * Returns: 0
 * Side Effects: None
 */
int32_t terminal_close(int32_t fd) {
    return 0;
}
