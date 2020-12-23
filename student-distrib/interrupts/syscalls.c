#include "syscalls.h"
#include "../lib.h"
#include "syscall_linkage.h"
#include "../filesystem.h"
#include "../page.h"
#include "../x86_desc.h"
#include "../devices/keyboard.h"
#include "../devices/rtc.h"
#include "../schedule.h"

typedef uint32_t function();

/* Jump tables for file operations based on file type */
uint32_t stdin_jmp[NUM_OPS] = {NULL, (uint32_t) terminal_read, (uint32_t)terminal_open, (uint32_t)terminal_close};
uint32_t stdout_jmp[NUM_OPS] = {(uint32_t) terminal_write, NULL, (uint32_t)terminal_open, (uint32_t)terminal_close};
uint32_t file_jmp[NUM_OPS] = {(uint32_t) fs_write, (uint32_t) fs_read, (uint32_t)fs_open, (uint32_t)fs_close};
uint32_t dir_jmp[NUM_OPS] = {(uint32_t) dir_write, (uint32_t) dir_read, (uint32_t)dir_open, (uint32_t)dir_close};
uint32_t rtc_jmp[NUM_OPS] ={(uint32_t) rtc_write, (uint32_t) rtc_read, (uint32_t)rtc_open, (uint32_t)rtc_close};

uint32_t * table_list[NUM_JMP_TABLES] = {rtc_jmp, dir_jmp, file_jmp}; /* Array of required jump tables */
int8_t pid_list[MAX_PROCESSES] = {NOT_IN_USE, NOT_IN_USE, NOT_IN_USE, NOT_IN_USE, NOT_IN_USE, NOT_IN_USE}; /* List of process usage */

/* fs_read()
 * Description: Reads data from file fd of current process.
 * Inputs: fd - index into file array of current process
 *         buf - buffer to read data into
 *         nbytes - number of bytes to read
 * Outputs: none
 * Returns: Number of bytes read
 * Side Effects: updates current position in file
 */
int32_t fs_read(int32_t fd, void* buf, int32_t nbytes){
    PCB * get_pcb = (PCB *) (_8MB - (_8KB*(pid + 1))); /* Get current PCB */
    
    /* Ensure location fd is open */
    if(get_pcb->file_ops[fd].flags == NOT_IN_USE){
        return 0; 
    }

    /* Read data from file */
    int read = read_data(get_pcb->file_ops[fd].inode, get_pcb->file_ops[fd].file_pos, (uint8_t *) buf, nbytes);
    if(read != -1) {
        /* Increment file location */
        get_pcb->file_ops[fd].file_pos += read; 
    }
    else {
        /* Return 0 if no bytes were read */
        return 0; 
    }
    /* Return number of bytes read */
    return read; 
}

/* halt()
 * Description: Ends current process and returns to parent
 *              process. Clears PCB and kernel stack.
 * Inputs: status - value to return to the parent process.
 *                  0-255 indicate a normal halt
 *                  256 indicates an exception
 * Outputs: none
 * Returns: 0, but should never do so
 * Side Effects: Returns to parent process
 */
int32_t halt(uint8_t status){
    /* Don't want anything to interrupt the halt */
    cli();

    /* mark process as unused in array */
    pid_list[(uint8_t)pid] = NOT_IN_USE; 

    /* Get the current process that we're halting */
    PCB *child = current_process->pcb; 

    /* Mark open files as closed, clear the PCB's command line, decrement total_processes */
    int j = 0;
    while (j < MAX_FILES) {
        close(j);
        j++;
    }
    clear_buffer(child->cmd_line, MAX_BUFF_LEN);
    total_processes--;
    
    /* Make sure shell is always running */
    if(current_process->parent == current_process) {
        total_base--;
        execute((uint8_t*)"shell");
    }

    /* Remap 128MB to parent's physical address and update the global PID */
    pid = child->parent->pid;
    vmap(_128MB, child->parent->pid); 

    /* Call relevant scheduling functions to update processes array and linked list */
    start_process(&(processes[child->parent->pid]));
    remove_process(&(processes[child->pid]));

    /* Switch context and restore parent's ESP/EBP*/
    tss.esp0 = child->parent_esp;          
    asm volatile(   "xorl %%eax, %%eax;"
                    "movb %0, %%al;"
                    "movl %1, %%esp;"
                    "movl %2, %%ebp;"
                    "sti;"
                    "leave;"
                    "ret;"
                    :
                    : "r"(status), "r"(child->parent_esp), "r"(child->parent_ebp)
                    : "%eax"
    );

    /* Shouldn't reach here */
    return 0; 
}

/* exec_halt()
 * Description: Ends current process and returns to parent
 *              process. Clears PCB and kernel stack.
 *              Only for processes that end by exception.
 * Inputs: status - value to return to the parent process.
 *                  256 indicates an exception
 * Outputs: none
 * Returns: 0, but should never do so
 * Side Effects: Returns to parent process
 */
int32_t exec_halt(uint32_t status){
    pid_list[(uint8_t) pid] = NOT_IN_USE;
    PCB * child = ((PCB *) (_8MB - _8KB*(pid + 1)));
    pid = ((PCB *) (_8MB - _8KB*(pid + 1)))->parent->pid; 
    int j = 0;
    while (j < MAX_FILES) {
        close(j);
        j++;
    }
    vmap(_128MB, child->parent->pid);
    tss.esp0 = child->parent_esp;
    total_processes--;
    remove_process(&(processes[child->pid]));
    start_process(&(processes[child->parent->pid]));

    asm volatile(   "xorl %%eax, %%eax;"
                    "movl %0, %%eax;"
                    "movl %1, %%esp;"
                    "movl %2, %%ebp;"
                    "leave;"
                    "ret;"
                    :
                    : "r"(status), "r"(child->parent_esp), "r"(child->parent_ebp)
                    : "%eax"
    );
    return 0;
}

/* execute()
 * Description: Creates a child process and PCB.  Starts running
 *              the child process.  Completely changes the context
 *              and allocates memory for child process.
 * Inputs: command - name of the executable
 * Outputs: none
 * Returns: -1 on failure, -2 when excess processes, 0-255 on success, and 256 if an exception was generated.
 * Side Effects: none
 */
int32_t execute(const uint8_t * command) {
    /* Don't execute a command if it causes process overflow */
    if(total_processes >= MAX_PROCESSES) {
        return -2;
    }

    /* Don't want interrupts to occur during execute logic */
    cli(); 

    uint8_t buf[NUM_OF_MAGIC_CHARS];   /* Used for checking exec. magic string           */
    char *temp = "ELF";                /* Magic word indicating a file is an executable  */
    dentry_t dentry_temp;              /* Dentry to fill with executable                 */
    dentry_t * dentry = &dentry_temp;  /* Pointer to dentry                              */
    uint8_t * dest;                    /* Pointer to physical address for instructions   */
    int vret;                          /* Return value of vmap                           */
    int32_t data_ret;                  /* Return value of read_data                      */
    int32_t not_same;                  /* Return value of strncmp                        */
    int n;                             /* Return value of read_data                      */
    uint32_t v_first;                  /* Virtual address of first instruction           */
    int j;                             /* General use integer                            */
    int32_t ret;                       /* Return for read_dentry_by_name                 */
    PCB *pcb;                          /* Pointer to new PCB                             */

    /* Copy actual command from buffer into copy_cmd */
    uint8_t i = 0;
    while(command[i] == ' '){
        i++;
    }
    uint8_t copy_idx = 0;
    uint8_t copy_cmd[MAX_FILE_LEN]; 
    while(command[i] != ' ' && command[i] != '\0' && command[i] != ASCII_NEWLINE) {
        copy_cmd[copy_idx] = command[i];
        i++;
        copy_idx++; 
    }

    /* terminate the cmd with EOS */
    copy_cmd[copy_idx] = '\0';

    /* Copy the arguments into this array, don't want garbage values inside the PCB's command line */
    uint8_t arguments[MAX_BUFF_LEN]; 
    clear_buffer(arguments, MAX_BUFF_LEN);
    parse(command, arguments, i, MAX_BUFF_LEN);

    /* Copy dentry for the command into dentry */
    ret = read_dentry_by_name((const uint8_t *) copy_cmd, dentry);

    /* Ensure read was successful */
    if (ret == -1) {
        sti();
        return -1;
    }
    /* executables have filetype 2 */
    if(dentry->f_type != EXEC_TYPE) {
        sti();
        return -1;
    }

    /* Read first 4 bytes of data into buf with 0 offset */
    data_ret = read_data(dentry->inode, 0, buf, 4);

    /* Ensure the data corresponds to the magic word,
     * indicating that the file is a valid executable.    */
    if(data_ret != NUM_OF_MAGIC_CHARS) {
        sti();
        return -1;
    }
    /* check to see if magic string is present: ASCII_DEL,E,L,F */
    /* checks to see if start of buff is ASCII_DEL */
    if(buf[START] != ASCII_DEL) {
        sti();
        return -1;
    }
    /* compare buf[3:1] to "ELF" to validate the string. Pass in &buf[1] to start at 2nd elem. in buffer, pass in 3 to indicate we want to check 3 chars */
    not_same = strncmp((const int8_t *) temp, (const int8_t *) &buf[1], 3);
    if(not_same != 0) {
        sti();
        return -1;
    }

    /* Generate PCB for new process */
    pcb = createPCB();
    if(pcb == NULL) {
        pid_list[(uint8_t) pid] = NOT_IN_USE;
        return -1;
    }

    /* Copy arguments into pcb */
    int x = 0;
    for(x = 0; x < MAX_BUFF_LEN; x++) {
        pcb->cmd_line[x] = arguments[x];
    }

    /* Map the process to virual memory */
    vret = vmap(_128MB, pid); 
    if(vret != 0) {
        sti();
        return -1;
    }

    /* Read the executable instructions to memory */
    dest = (uint8_t*) (_128MB + EXEC_OFFSET);
    n = read_data(dentry->inode, 0, (uint8_t*) dest, (inodes + dentry->inode)->length); 
    /* validate successful read */
    if(n == -1 || n == 0) {
        sti();
        return -1;
    }

    /* Locate the first instruction */
    v_first = 0;
    /* byte 27 is the msbyte of loc. of instruction, so work backwards 24 + 3, 24 + 2...*/
    for(j = 3; j >= 0; j--) {
        /* left shift 8 to make space for next byte */
        v_first  = v_first << 8; 
        v_first += dest[INSTR_START + j];
    }

    /* Create and add process */
    process_t process;
    process.terminal = &(terminals[curr_tid]);
    process.pid = pid;
    process.pcb = pcb; 
    /* subtract 4 bytes to get pointer into valid kernel stack range (can't be 8 MB, 12MB, so subtract 4 instead of 1 to keep it aligned) */
    process.esp0 = _8MB - (_8KB * (pcb->pid)) - 4;

    /* The first 3 shells (base shells) are parents to themselves, otherwise the parent is the current process that executed a command */
    if(total_base < MAX_TERMINALS) {
        process.parent = &processes[pid];
        process.next = processes[pid].next;
        total_base++;
    }
    else {
        process.parent = &processes[current_process->pid];
    }

    /* init the ebp esp and next to point to itself in the start */
    if(total_processes == START) {
        process.esp = processes[START].esp;
        process.ebp = processes[START].ebp;
        process.next = &processes[START];
    }

    /* add process to scheduler and start it */
    add_process(&process);
    start_process(&process);

    /* Context switch */
    tss.ss0 = KERNEL_DS;
    /* subtract 4 bytes to get pointer into valid kernel stack range (can't be 8 MB, 12MB, so subtract 4 instead of 1 to keep it aligned) */
    tss.esp0 = _8MB - (_8KB * (pcb->pid)) - 4;

    /* Get current esp / ebp and store into pcb */
    asm volatile("movl %%esp, %0":"=g"(pcb->parent_esp));
    asm volatile("movl %%ebp, %0":"=g"(pcb->parent_ebp));

    /* Increment number of processes */
    total_processes++; 

    /* Set up stack as if an interrupt was generated by the new process and iret, causing the executable to run */
    asm volatile(
        "cli;"
        "movw $0x2B, %%bx;" /* USER_DS */
        "movw %%bx, %%ds;"
        "pushl $0x2B;"
        "pushl $0x83FFFFC;" /* 132MB - 4 bytes*/
        "pushfl;"
        "popl %%ebx;"
        "orl $0x200, %%ebx;"/* set the IF flag*/
        "pushl %%ebx;"
        "pushl $0x23;"      /* USER_CS */
        "pushl %0;"         /* address of first instruction */
        "iret;"
        :
        : "r"(v_first)
        : "%ebx"
    );

    return 0; /* Shouldn't ever reach here */
}

/* read()
 * Description: System call read which calls a helper read function
 *              based on the file type which read data from a file to
 *              a buffer.
 * Inputs: fd     - index into current PCB's file array
 *         buf    - location to read file data to
 *         nbytes - number of bytes to read
 * Outputs: none
 * Returns: number of bytes read, -1 on failure
 * Side Effects: none
 */
int32_t read (int32_t fd, void* buf, int32_t nbytes){
    /* ensure fd is valid */
    if(fd < MIN_FILES || fd >= MAX_FILES || buf == NULL) {
        return -1;
    }

    /* get the current pcb */
    PCB * get_pcb = (PCB *) (_8MB - _8KB*(pid + 1));

    /* make sure pcb entry for fd is valid */
    if(get_pcb->file_ops[fd].flags == NOT_IN_USE) {
        return -1;
    }
     if(get_pcb->file_ops[fd].func_ptr[READ] == NULL) {
        return -1;
    }

    /* Call and return helper function based on file */
    return  ((function *)  (get_pcb->file_ops[fd].func_ptr[READ])) (fd, (char *) buf, nbytes);
}

/* write()
 * Description: System call write which calls a helper write function
 *              based on the file type.  None of which do anything yet.
 * Inputs: fd     - index into current PCB's file array
 *         buf    - location to write file data from
 *         nbytes - number of bytes to write
 * Outputs: none
 * Returns: number of bytes written, -1 on failure
 * Side Effects: none
 */
int32_t write (int32_t fd, const void* buf, int32_t nbytes){
    /* Ensure fd is valid */
    if(fd < MIN_FILES || fd >= MAX_FILES || buf == NULL){
        return -1;
    }

    /* Get current PCB */
    PCB * get_pcb = (PCB *) (_8MB - _8KB*(pid + 1));

    /* Ensure pcb entry for fd is valid */
    if(get_pcb->file_ops[fd].func_ptr[WRITE] == NULL){
        return -1;
    }

    /* Call and return helper function based on file */
    return  ((function *)  (get_pcb->file_ops[fd].func_ptr[WRITE])) (fd, (char *) buf, nbytes);
}

/* open()
 * Description: System call open which opens a file for the
 *              current process if there is space available.
 * Inputs: filename - name of file to open
 * Outputs: none
 * Returns: index into file array, -1 on failure
 * Side Effects: none
 */
int32_t open (const uint8_t* filename){
    PCB * curr = (PCB *) (_8MB - (_8KB* (pid + 1))); /* Get the current PCB */
    int index = 0; /* index into file array of PCB */
    dentry_t dentry1; /* dentry to copy file information into */
    dentry_t * dentry = &dentry1;
    int ret;
  
    /* Read dentry of file into dentry */
    ret = read_dentry_by_name(filename, dentry);

    if(ret != 0){
        return -1;
    }

    int filetype = dentry->f_type;

    /* look for open space in file array and copy information */
    while(index < MAX_FILES){
        if(curr->file_ops[index].flags == NOT_IN_USE){
            curr->file_ops[index].flags = IN_USE;
            curr->file_ops[index].func_ptr = table_list[filetype];
            ret = ((function *) curr->file_ops[index].func_ptr[OPEN]) (filename);
            if (ret != 0) {
                return -1; 
            }
            if(filetype == EXEC_TYPE) {
                curr->file_ops[index].inode = dentry->inode;
            } 
            else {
                curr->file_ops[index].inode = START;
            }
            curr->file_ops[index].file_pos = START;
            return index; /* Return index where file is in array */
        }
        index++;
    }
    /* Return -1 if no space was found */
    return -1;
}

/* close()
 * Description: System call which marks a file as closed
 *              in the pcb.
 * Inputs: fd - index into current PCB's file array to close
 * Outputs: none
 * Returns: 0 on success, -1 on failure
 * Side Effects: none
 */
int32_t close (int32_t fd){
    /* Ensure fd is valid (can't close stdin/out) */
    if(fd == STD_OUT || fd == STD_IN || fd < MIN_FILES || fd >= MAX_FILES){
        return -1;
    }

    /* get current PCB */
    PCB * curr = current_process->pcb;

    /* Look for file and mark as closed if opened */
    if(curr->file_ops[fd].flags == IN_USE){
        curr->file_ops[fd].flags = NOT_IN_USE;
        return 0;
    }

    /* Return -1 if the file was not open */
    return -1;
}

/* getargs()
 * Description: Writes the arguments of the current process into
 *              the input buffer.
 * Inputs: buf - destination for args
 *         nbytes - number of bytes to copy
 * Outputs: none
 * Returns: 0 on success, -1 on failure
 * Side Effects: none
 */
int32_t getargs (uint8_t* buf, int32_t nbytes){
    /* Make sure buf pointer is valid */
    if(buf == NULL){
        return -1;
    }

    /* Get current pcb */
    PCB * curr = current_process->pcb;

    /* Copy arguments */
    int i;
    /* Empty args is failure */
    if (curr->cmd_line[START] == '\0') {
        return -1; 
    }
    for(i = 0; i < nbytes; i++){
        buf[i] = curr->cmd_line[i];
        if(buf[i] == '\0') {
            break;
        }
    }
    return 0;
}

/* vidmap()
 * Description: Maps input user pointer to start of video memory.
 * Inputs: screen_start - pointer to set
 * Outputs: none
 * Returns: 0 on success, -1 on failure
 * Side Effects: none
 */
int32_t vidmap (uint8_t** screen_start){
    /* Ensure input pointer is valid */
    if(screen_start == NULL || (int)screen_start < _128MB || (int)screen_start >= (_128MB + _4MB)){
        return -1;
    }

    /* Change necessary paging and flush the TLB afterwards */
    Page_Directory[USR_VIDEO_PDE] = ((uint32_t) Page_Table) | PRESENT | R_W | User_SUP;
    /* if vidmap called on currently visible terminal, associate Page_Table[PTE] with displayed video mem */
    if(current_process && current_process->terminal->tid == curr_tid) {
        Page_Table[VIDEO_PTE] = (uint32_t)video_mem | PRESENT | R_W | User_SUP;
    }
    else {
        /* otherwise associate it with the background terminal's vmem buffer */
        Page_Table[VIDEO_PTE] = (uint32_t)(current_process->terminal->vmem) | PRESENT | R_W | User_SUP;
    }
    flush_TLB();

    *screen_start = (uint8_t*) (_128MB + _4MB); 
    return 0;
}

/* vmap()
 * Description: Maps an input process and virtual address
 *              to a page in the pd
 * Inputs: vaddr - virtual address
 *         pid - pid of process being mapped
 * Outputs: none
 * Returns: 0 on success, -1 on failure
 * Side Effects: Overwrites current page for given pid
 */
int vmap(uint32_t vaddr, uint32_t pid_) {
    /* calculate phys addr based on vaddr and pid */
    /* right shift 22 to get top 10 msb as index into page_directory */
    uint32_t temp = vaddr >> 22;
    uint32_t paddr = pid_ * _4MB + _8MB;
    
    /* Map phys addr to page and flush the TLB afterwards */
    Page_Directory[temp] = paddr | PRESENT | R_W | PS | User_SUP;
    flush_TLB();

    return 0;
}

/* createPCB()
 * Description: Create a PCB with the lowest available pid
 * Inputs: none
 * Outputs: none
 * Returns: pointer to new PCB
 * Side Effects: Overwrites old data at that pid's PCB location
 */
PCB * createPCB() {
    /* Get new pid and ensure its validity */
    int8_t temp_pid = pid;
    int8_t temp = get_pid();
    if(temp == -1) {
        return NULL;
    }
    /* Set current pid to new pid */ 
    pid = temp;    

    /* Calculate location of pcb based on pid */
    PCB * pcb = (PCB *) (_8MB - _8KB*(pid + 1));

    /* Fill pcb with relevant values */
    pcb->pid = pid;

    /* The first base shell's pcb parents are themselves */
    if(pid < MAX_TERMINALS) {
        pcb->parent = pcb;
    }
    else {
        pcb->parent = (PCB *) (_8MB - _8KB*(temp_pid + 1));
    }
    
    /* Init the FD array */
    /* All processes have at least standard in and standard out open intially */
    pcb -> file_ops[STD_IN].func_ptr = stdin_jmp; 
    pcb -> file_ops[STD_IN].flags = IN_USE;
    pcb -> file_ops[STD_OUT].func_ptr = stdout_jmp;
    pcb -> file_ops[STD_OUT].flags = IN_USE;
    int j;
    /* All other files are not in use upon init */
    for(j = STD_OUT + 1; j < MAX_FILES; j++) {
        pcb->file_ops[j].func_ptr = NULL; 
        pcb->file_ops[j].flags = NOT_IN_USE;
    }
    /* Make sure there's no garbage in the PCB's command line buffer */
    clear_buffer(pcb->cmd_line, MAX_BUFF_LEN);
    return pcb;
}

/* get_pid()
 * Description: find and return the lowest available pid
 * Inputs: none
 * Outputs: none
 * Returns: pid
 * Side Effects: none
 */
int8_t get_pid() {
    /* Iterate through PID array and return available PID */
    int i;
    for(i = 0; i < MAX_PROCESSES ; i++) {
        if(pid_list[i]== NOT_IN_USE) {
            pid_list[i] = IN_USE;
            return i;
        }
    }
    /* Return -1 if none are available and maxes processes are in progress */
    return -1;
}

/* parse()
 * Description: Fill a buffer with all of the arguments corresponding
 *              to a command.
 * Inputs: buf    - the buffer that contains the arguments to parse
 *         buf2   - the buffer to write the arguments to
 *         i      - index into buf to copy from
 *         length - length of buffers
 * Outputs: none
 * Returns: none
 * Side Effects: Overwrites old data in buf2
 */
void parse(const uint8_t * buf, uint8_t * buf2, int i, int length) {
    /* Validate both buffers */
    if(buf == NULL || buf2 == NULL) {
        return; 
    }
    if(buf[i] == ' ') {
        /* Ignore leading white space */
        while(buf[i] == ' ') {
            i++;
        }
        /* Copy first non white-space character and onwards into target buffer (buf2) */
       int copy_idx = 0;
       
       /* Make sure we don't go passed valid length and only copy until EOS or newline */
       while((i < length) && (buf[i] != '\0') && (buf[i] != ASCII_NEWLINE)) {
            buf2[copy_idx] = buf[i];
            i++;
            copy_idx++;
        }
        buf2[copy_idx] = '\0';
    }
}
