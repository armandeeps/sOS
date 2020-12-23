/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* i8259_init()
 * Description: Initialize PIC
 * Inputs: none
 * Outputs: none
 * Returns: none
 * Side Effects: None
 */
void i8259_init(void) {
	outb(MASK_ALL, MASTER_DATA_PORT); /* Mask all of Master */
	outb(MASK_ALL, SLAVE_DATA_PORT);  /* Mask all of Slave  */

	/* Send the 4 ICWs to Master */
	outb(ICW1, MASTER_8259_PORT);
	outb(ICW2_MASTER, MASTER_DATA_PORT);
	outb(ICW3_MASTER, MASTER_DATA_PORT);
	outb(ICW4, MASTER_DATA_PORT);

	/* Send the 4 ICWs to Slave */
	outb(ICW1, SLAVE_8259_PORT);
	outb(ICW2_SLAVE, SLAVE_DATA_PORT);
	outb(ICW3_SLAVE, SLAVE_DATA_PORT);
	outb(ICW4, SLAVE_DATA_PORT);

	outb(MASK_ALL, MASTER_DATA_PORT); /* Mask all of Master */
	outb(MASK_ALL, SLAVE_DATA_PORT);  /* Mask all of Slave  */

}

/* enable_irq()
 * Description: Enable (unmask) the specified IRQ.
 *				OSDev Inspired.
 * Inputs: irq_num - the interrupt request number to unmask
 * Outputs: none
 * Returns: none
 * Side Effects: None
 */
void enable_irq(uint32_t irq_num) {
	uint8_t curr_mask;
	uint8_t data_port;

	/* Check that irq_num is valid */
	if(irq_num > IRQ_MAX || irq_num < IRQ_MIN){
		return;
	}

	/* Select slave or master data port based on irq */
	if(irq_num < IRQ_MAX_MASTER) {
		data_port = MASTER_DATA_PORT;
	}
	else{
		enable_irq(SLAVE_IRQ);                /* Enable slave irq */ 
		data_port = SLAVE_DATA_PORT;
		irq_num = irq_num - IRQ_MAX_MASTER;   /* Individual PIC IRQ ranges from 0 - 7 */
	}

	curr_mask = inb(data_port);  /* Get the current mask on PIC */

	/* Unmask (set to 0) selected IRQ's bit and send new mask to PIC */
	curr_mask = curr_mask & ~(1 << irq_num);
	outb(curr_mask, data_port);
}


/* disable_irq()
 * Description: Disnable (mask) the specified IRQ.
 *				OSDev Inspired.
 * Inputs: irq_num - the interrupt request number to mask
 * Outputs: none
 * Returns: none
 * Side Effects: None
 */
void disable_irq(uint32_t irq_num) {
	uint8_t curr_mask;
	uint8_t data_port;

	/* Check that irq_num is valid */
	if(irq_num > IRQ_MAX || irq_num < IRQ_MIN){
		return;
	}

	/* Select slave or master data port based on irq */
	if(irq_num < IRQ_MAX_MASTER){
		data_port = MASTER_DATA_PORT;
	}
	else{
		data_port = SLAVE_DATA_PORT;
		irq_num = irq_num - IRQ_MAX_MASTER;  /* Individual PIC IRQ ranges from 0 - 7 */
	}

	curr_mask = inb(data_port);  /* Get the current mask on PIC */

	/* Mask (set to 1) selected IRQ's bit and send new mask to PIC */
	curr_mask = curr_mask | (1 << irq_num);
	outb(curr_mask, data_port);
}

/* disable_irq()
 * Description: Send end-of-interrupt signal for the specified IRQ.
 *				OSDev Inspired.
 * Inputs: irq_num - the interrupt request number to alert
 * Outputs: none
 * Returns: none
 * Side Effects: None
 */
void send_eoi(uint32_t irq_num) {
	int data;

	/* Check that irq_num is valid */
	if(irq_num > IRQ_MAX || irq_num < IRQ_MIN){
		return;
	}

	/* Check if IRQ belongs to slave or master */
	if(irq_num < IRQ_MAX_MASTER){
		/* Or irq with EOI and send it to Master PIC */
		data = irq_num | EOI; 
		outb(data, MASTER_8259_PORT);
	}
	else{
		/* Or irq with EOI and send to Slave PIC */
		data = (irq_num - IRQ_MAX_MASTER) | EOI; /* Each PICs irq ranges from 0-7 */
		outb(data, SLAVE_8259_PORT);

		/* EOI must also be sent to master */
		data = SLAVE_IRQ | EOI;
		outb(data, MASTER_8259_PORT);
	}   
}
