#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "i8259.h"
#include "../types.h" 

#define KEYBOARD_IRQ 1
#define NUM_KEYS 60
#define STATES 4 
#define CAPS_PRESSED  2
#define SHIFT_PRESSED 1 
#define L_SHIFT_RELEASED 0xAA
#define R_SHIFT_RELEASED 0xB6
#define RELEASED 0
#define PRESSED  1 
#define PS2_DATA_PORT 0x60  
#define PS2_CMD_PORT  0x64
#define BUFF_SIZE 128
#define L_SHIFT   0x2A
#define R_SHIFT   0x36
#define CAPS_LOCK 0x3A
#define ENTER 0x1C
#define CTRL_PRESSED  0x1D
#define CTRL_RELEASED 0x9D
#define ALT_PRESSED   0x38
#define ALT_RELEASED  0xB8
#define L  0x26
#define F1 0x3B
#define F2 0x3C
#define F3 0x3D
#define BACKSPACE 0x0E
#define TERM0 0
#define TERM1 1
#define TERM2 2

/* keyboard handling functions */
extern void buffer_backspace();
extern void clear_buffer(unsigned char*, uint32_t);
extern void keyboard_handler();
extern void init_keyboard();
extern void disable_keyboard();

/* kb_buff holds all the key presses, buff_idx is the next available point in the buffer */
volatile int buff_idx;
volatile uint8_t * kb_buff;

#endif
