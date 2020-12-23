/* LINKAGE()
 * Description: General hardware-interrupt assembly linkage, saves all regs and flags to restore after interrupt
 * Inputs: device_name - hardware linker function
 *         handler_name - interrupt handler code 
 * Outputs: none
 * Returns: none
 * Side Effects: Modifies the stack before call to handler_name, restores it afterwards before iret
 */
#define LINKAGE(device_name, handler_name)  \
.globl device_name              ;\
device_name:                    ;\
pushal                          ;\
pushfl                          ;\
call handler_name               ;\
popfl                           ;\
popal                           ;\
iret                           

LINKAGE(keyboard, keyboard_handler);
LINKAGE(RTC, rtc_handler);
LINKAGE(PIT, pit_handler);
