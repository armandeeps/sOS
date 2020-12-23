
#include "rtc.h"
#include "i8259.h"
#include "../lib.h"
#include "../schedule.h"

/* init_rtc()
 * Description: Initialize the RTC by alerting RTC to start ticking.
 *              Inspired by OSDev.
 * Inputs: none
 * Outputs: none
 * Returns: none
 * Side Effects: None
 */
void init_rtc() {	
	outb(RTC_REG_B, RTC_CMD_PORT);     /* Disable NMIs and select register B of rtc    */
	char prev = inb(RTC_DATA_PORT);	   /* Read register B of rtc                       */
	outb(RTC_REG_B, RTC_CMD_PORT);	   /* Reselect register B of rtc                   */
	outb(prev | 0x40, RTC_DATA_PORT);  /* Turn on bit 6 of register B                  */
	
	uint8_t	rate = HARDWARE_RATE;			   /* Frequency = 32768 >> (rate-1), with rate = 6, f = 1024hz                    */
	outb(RTC_REG_A, RTC_CMD_PORT);     		   /* Disable NMIs and select register A of rtc                                   */
	prev = inb(RTC_DATA_PORT);	   			   /* Get initial register A value                                                */
	outb(RTC_REG_A, RTC_CMD_PORT);	   		   /* Reselect register A of rtc                                                  */
	outb((prev & 0xF0) | rate, RTC_DATA_PORT); /* Keep initial most significant 4 bits, and send updated rate (bottom 4 bits) */
	
	enable_irq(RTC_IRQ);
}

/* rtc_handler()
 * Description: Handle RTC requests and alert RTC when finished.
 * Inputs: none
 * Outputs: none
 * Returns: none
 * Side Effects: none
 */
void rtc_handler() {
	// test_interrupts();
	/* Select register C and dump its value in order for rtc to interrupt again */
	outb(RTC_REG_C, RTC_CMD_PORT);  
	inb(RTC_DATA_PORT); 
	int i;
	for(i = 0; i < MAX_TERMINALS; i++) {
		/* Update all the terminal's virtualized rtc */
		terminals[i].term_rtc_counter--; 
		if(terminals[i].term_rtc_counter == 0) {
			terminals[i].term_rtc_counter = HARDWARE_FREQ / terminals[i].term_freq;
			terminals[i].term_rtc_flag = CLEAR; 
		}
	} 
	send_eoi(RTC_IRQ);
	return;
}

/* rtc_open()
 * Description: Set RTC frequency to 2hz and open it as a file.
 * Inputs: filename - nothing yet
 * Outputs: none
 * Returns: none
 * Side Effects: none
 */
int32_t rtc_open(const uint8_t * filename) {
	current_process->terminal->term_freq = DEFAULT_FREQ;
	current_process->terminal->term_rtc_counter = HARDWARE_FREQ / current_process->terminal->term_freq;
	return 0;
}

/* rtc_close()
 * Description: Should close RTC as a file.  Not implemented yet.
 * Inputs: fd - 
 * Outputs: none
 * Returns: 0
 * Side Effects: none
 */
int32_t rtc_close(int32_t fd) {
	return 0;
}

/* rtc_read()
 * Description: Counts hardware interrupts and waits to generate virtual
 *              interrupt. Not implemented yet
 * Inputs: fd - file directory (ignore for now)
 *         buf - 
 *         nbytes - 
 * Outputs: none
 * Returns: 0
 * Side Effects: none
 */
int32_t rtc_read(int32_t fd, void * buf, int32_t nbytes) {
	current_process->terminal->term_rtc_flag = SET;
	while(current_process->terminal->term_rtc_flag);
	return 0;
}

/* rtc_write()
 * Description: Set the rtc fequency to the input frequency.
 * Inputs: fd - file directory (ignore for now)
 *         buf - pointer to desired frequency
 *         nbytes - length of buffer
 * Outputs: none
 * Returns: 0 on success, -1 otherwise
 * Side Effects: none
 */
int32_t rtc_write(int32_t fd, void * buf, int32_t nbytes){
	if(nbytes != INT_BYTE_SIZE || buf == NULL) {
		return -1;
	}

	int32_t f = *((int32_t *) buf);
	/* check for valid buf values. Power of 2, not 0, and not greater than 1024 (max qemu can handle) */
	if((f & (f-1)) != 0 || f == 0 || f > HARDWARE_FREQ) {
		return -1;
	}

	/* update the freq and counter of the current process that called rtc_write */
	current_process->terminal->term_freq = f; 
	current_process->terminal->term_rtc_counter = HARDWARE_FREQ / f; 
	return 0;
}
