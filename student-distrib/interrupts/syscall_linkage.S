/* sys_call()
 * Description: System call interrupt assembly linkage
 * Inputs: eax         - syscall number
 *         edx,ecx,ebx - arguments right to to left respectively
 * Outputs: syscall return value, -1 if failed (either syscall function failed, or invalid syscall number was passed in)
 * Returns: none
 * Side Effects: eax modified 
 */
.globl sys_call 
sys_call:
    pushl %edi
    pushl %esi
    pushl %ebx

    cmpl $0, %eax
    jle INVALID

    cmpl $10, %eax
    jg INVALID

    pushl %edx
    pushl %ecx 
    pushl %ebx
    call *sys_call_table(, %eax, 4)
    popl %ebx
    popl %ecx
    popl %edx


    popl %ebx
    popl %esi
    popl %edi
    iret

INVALID:
    popl %ebx
    popl %esi
    popl %edi
    movl $-1, %eax
    iret

sys_call_table:
    .long 0, halt, execute, read, write, open, close, getargs, vidmap, mmap

