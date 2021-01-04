#include "mouse.h"
#include "i8259.h"
#include "../lib.h"
#include "keyboard.h"
#include "../types.h"
#include "../terminal.h"

void init_mouse()
{
	mouse_x = 0;
	mouse_y = 0;
	last_vmem = video_mem;
	// int loc = NUM_COLS * mouse_display_y + mouse_display_x;
 //    *(uint8_t *)(last_vmem + (loc << 1) + 1) = (ATTRIB & 0x0F | 0xC0);

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
	//restore attrib of current mouse_display_x/y
	int loc = NUM_COLS * mouse_display_y + mouse_display_x;
    *(uint8_t *)(last_vmem + (loc << 1) + 1) = ATTRIB;

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

	int delta_x = code[1];
	int delta_y = code[2];

	if(code[0] & 0xC0)
	{
		send_eoi(MOUSE_IRQ);
		return;
	}

	if(!(code[0] & 0x20))
	{
		delta_y |= 0xFFFFFF00;
	}
	if((code[0] & 0x10))
	{
		delta_x |= 0xFFFFFF00;
	}

	mouse_x += delta_x; //4 per mm
	mouse_y += delta_y;

	if(mouse_x < 0)
	{
		mouse_x = 0;
	}
	if(mouse_x > NUM_COLS)
	{
		mouse_x = NUM_COLS-1;
	}

	if(mouse_y < 0)
	{
		mouse_y = 0;
	}
	if(mouse_y > 1000*NUM_ROWS)
	{	
		mouse_y = 1000*NUM_ROWS-1;
	}

	mouse_display_x = mouse_x;
	printf("~\n%d\n", mouse_x);
	mouse_display_y = mouse_y / 1000;
	printf("%d\n", mouse_display_y);

	//set new attrib of mouse_display_x/y
	loc = NUM_COLS * mouse_display_y + mouse_display_x;

	last_vmem = video_mem;
    *(uint8_t *)(last_vmem + (loc << 1) + 1) = (ATTRIB & 0x0F | 0xC0);

	// kb_putc(code[0]);
	// kb_putc(code[1]);
	// kb_putc(code[2]);

	send_eoi(MOUSE_IRQ);
}
