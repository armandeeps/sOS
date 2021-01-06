#ifndef MOUSE_H
#define MOUSE_H

#include "../types.h"

#define MOUSE_IRQ    12
#define MOUSE_STREAM 0xF4
#define MOUSE_ACK    0xFA
#define MOUSE_RESET  0xFF
#define SELECT_MOUSE 0xD4
#define GET_COMPAQ_STAT 0x20
#define SET_COMPAQ_STAT 0x60
#define OVERFLOW  0xC0
#define MOVE_BIT  0x08
#define Y_SIGN    0x20
#define X_SIGN    0x10
#define X_SCALE   4
#define Y_SCALE   8
#define BUTTONS   0x07
#define NEGATE    0xFFFFFF00
#define FOREGROUND_MASK 0x0F
#define PEACH_COLOR     0xC0
#define RED_COLOR       0x40


extern void init_mouse();
extern void mouse_handler();
void write_mouse(uint8_t in);
void wait_mouse_read();
void wait_mouse_write();

int mouse_x;
int mouse_y;
int mouse_display_x;
int mouse_display_y;
uint8_t * last_vmem;

#endif
