#ifndef PIT_H
#define PIT_H

#define PIT_IRQ 0
#define PIT_CMD_PORT 0x43
#define PIT_DATA_0 0x40
#define PIT_ICW0 0x36
#define DESIRED_FREQ 100
#define NATURAL_FREQ 1193182
#define MAX_PROCESSES 6
#define LOWER_MASK 0x00FF
#define UPPER_MASK 0xFF00

/* PIT functions */
extern void init_PIT();
extern void pit_handler();

#endif
