#include "mouse.h"
#include "i8259.h"
#include "../lib.h"
#include "../page.h"
#include "keyboard.h"
#include "../types.h"
#include "../terminal.h"

void init_mouse()
{
	mouse_x = 0;
	mouse_y = 0;
	last_vmem = video_mem;

	cli();
	wait_mouse_write();
	outb(SELECT_MOUSE, PS2_CMD_PORT);
	wait_mouse_write();
	outb(MOUSE_RESET, PS2_DATA_PORT);

	wait_mouse_write();
	outb(SELECT_MOUSE, PS2_CMD_PORT);
	wait_mouse_write();
	outb(GET_COMPAQ_STAT, PS2_CMD_PORT);

	wait_mouse_read();
	uint8_t status = inb(PS2_DATA_PORT);

	status |= 2;    /* Enable aux bit            */
	status &= 0xDF; /* Disable mouse clock (0x20)*/

	wait_mouse_write();
	outb(SELECT_MOUSE, PS2_CMD_PORT);
	wait_mouse_write();
	outb(SET_COMPAQ_STAT, PS2_CMD_PORT);
	wait_mouse_write();
	outb(status, PS2_DATA_PORT);

	wait_mouse_write();
	outb(SELECT_MOUSE, PS2_CMD_PORT);
	wait_mouse_write();
	outb(MOUSE_STREAM, PS2_DATA_PORT);

	// OSDEV SET SAMPLING RATE:
	// outb(0xD4, 0x64);                    // tell the controller to address the mouse
	// outb(0xF3, 0x60);                    // write the mouse command code to the controller's data port
	// while(!(inb(0x64) & 1)); // wait until we can read
	// uint8_t ack = inb(0x60);                     // read back acknowledge. This should be 0xFA
	// outb(0xD4, 0x64);                    // tell the controller to address the mouse
	// outb(40, 0x60);                     // write the parameter to the controller's data port
	// while(!(inb(0x64) & 1)); // wait until we can read
	// ack = inb(0x60);                     // read back acknowledge. This should be 0xFA


    sti();
	enable_irq(MOUSE_IRQ);
}

void wait_mouse_read()
{	
	int i = 100000;
	while(i-- && inb(PS2_CMD_PORT) & 1 == 0);
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

uint8_t read_mouse_byte()
{
	if ((inb(PS2_CMD_PORT) & 0x1) == 0 ) {
        return 0;
    } else {
        return inb(PS2_DATA_PORT);
    }
}

void mouse_handler()
{
	//restore attrib of current mouse_display_x/y
	int loc = NUM_COLS * mouse_display_y + mouse_display_x;
    // *(uint8_t *)(last_vmem + (loc << 1) + 1) = ATTRIB;

	uint8_t code;
	cli();
	/* 
	 * Collect all 3 data packets from mouse 
	 * Byte 1 - | y overflow | x overflow | y sign | x sign | 1 | Middle btn | Right btn | Left btn |
	 * Byte 2 - delta X
	 * Byte 3 - delta Y
	 */
	code = read_mouse_byte();

	if(code & OVERFLOW || !(code & MOVE_BIT) || code == MOUSE_ACK)
	{
		send_eoi(MOUSE_IRQ);
		sti();
		return;
	}

	wait_mouse_read();
	int32_t delta_x = inb(PS2_DATA_PORT);
	wait_mouse_read();
	int32_t delta_y = inb(PS2_DATA_PORT);

	if((code & Y_SIGN))
	{
		delta_y |= NEGATE;
	}
	if((code & X_SIGN))
	{
		delta_x |= NEGATE;
	}

	mouse_x += delta_x; //4 per mm
	mouse_y -= delta_y;

	if(mouse_x < 0)
	{
		mouse_x = 0;
	}
	if(mouse_x >= X_SCALE*NUM_COLS)
	{
		mouse_x = X_SCALE*NUM_COLS-1;
	}

	if(mouse_y < 0)
	{
		mouse_y = 0;
	}
	if(mouse_y >= Y_SCALE*NUM_ROWS)
	{	
		mouse_y = Y_SCALE*NUM_ROWS-1;
	}

	if(mouse_x / X_SCALE != mouse_display_x || mouse_y / Y_SCALE != mouse_display_y)
	{
		*(uint8_t *)(VIDEO_MEMORY_START + (loc << 1) + 1) = ATTRIB;
	}

	mouse_display_x = mouse_x / X_SCALE;
	// printf("~\n%d\n", mouse_display_x);
	mouse_display_y = mouse_y / Y_SCALE;
	// printf("%d\n", mouse_display_y);

	//set new attrib of mouse_display_x/y
	loc = NUM_COLS * mouse_display_y + mouse_display_x;

	if (code & BUTTONS)
	{
		*(uint8_t *)(VIDEO_MEMORY_START + (loc << 1) + 1) = (ATTRIB & FOREGROUND_MASK | RED_COLOR);
	}
	else
	{
		*(uint8_t *)(VIDEO_MEMORY_START + (loc << 1) + 1) = (ATTRIB & FOREGROUND_MASK | PEACH_COLOR);
	}

	send_eoi(MOUSE_IRQ);
	sti();
}
