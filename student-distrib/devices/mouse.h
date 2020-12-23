#ifndef MOUSE_H
#define MOUSE_H

#include "../types.h"

#define MOUSE_IRQ    12
#define MOUSE_FREQ   40
#define MOUSE_CW1    0xF6
#define MOUSE_CW2	 0xF4
#define MOUSE_CW3    0xF3
#define MOUSE_ACK    0xFA
#define SELECT_MOUSE 0xd4
#define ENABLE_AUX   0xA8
#define GET_COMPAQ_STAT 0x20
#define SET_COMPAQ_STAT 0x60

extern void init_mouse();
extern void mouse_handler();
void write_mouse(uint8_t in);
void wait_mouse_read();
void wait_mouse_write();

#endif
