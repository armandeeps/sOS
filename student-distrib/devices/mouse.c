#include "mouse.h"
#include "i8259.h"
#include "../lib.h"
#include "keyboard.h"
#include "../types.h"

void init_mouse()
{
	cli();
	wait_mouse_write();
	outb(ENABLE_AUX, PS2_CMD_PORT);

	wait_mouse_write();
	outb(GET_COMPAQ_STAT, PS2_CMD_PORT);
	wait_mouse_read();
	uint8_t status = inb(PS2_DATA_PORT);
	status |= 2;
	status &= 0xDF;

	wait_mouse_write();
	outb(SET_COMPAQ_STAT, PS2_CMD_PORT);
	wait_mouse_write();
	outb(status, PS2_DATA_PORT);

	write_mouse((uint8_t) MOUSE_CW1);
	write_mouse((uint8_t) MOUSE_CW2);

	wait_mouse_write();
    outb(SELECT_MOUSE, PS2_CMD_PORT);
    wait_mouse_write();
    outb(MOUSE_CW3, PS2_DATA_PORT);
    wait_mouse_write();
    outb(MOUSE_FREQ, PS2_DATA_PORT);

    sti();
	enable_irq(MOUSE_IRQ);
}

void wait_mouse_read()
{	
	int i = 100000;
	while(i-- && inb(PS2_CMD_PORT) & 1);
}

void wait_mouse_write()
{
	int i = 100000;
	while(i-- && inb(PS2_CMD_PORT) & 2);
}

void write_mouse(uint8_t in)
{
	wait_mouse_write();
	outb(SELECT_MOUSE, PS2_CMD_PORT);
	wait_mouse_write();
	outb(in, PS2_DATA_PORT);
	wait_mouse_read();
	inb(PS2_DATA_PORT);
}

void mouse_handler()
{
	kb_putc('M');
	uint8_t code[3];
	cli();
	/* 
	 * Collect all 3 data packets from mouse 
	 * Byte 1 - | y overflow | x overflow | y sign | x sign | 1 | Middle btn | Right btn | Left btn |
	 * Byte 2 - delta X
	 * Byte 3 - delta Y
	 */
	code[0] = inb(PS2_DATA_PORT);
	code[1] = inb(PS2_DATA_PORT);
	code[2] = inb(PS2_DATA_PORT);
	sti();

	kb_putc(code[0]);
	kb_putc(code[1]);
	kb_putc(code[2]);

	send_eoi(MOUSE_IRQ);
}
