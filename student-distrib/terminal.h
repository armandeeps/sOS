#ifndef TERMINAL_H
#define TERMINAL_H

#include "types.h"
#include "schedule.h"
#define MAX_TERMINALS 3
#define BUFF_SIZE     128
#define WHITE         0x07
#define CYAN          0x0B
#define GREEN         0x02
#define _4KB		  4096

typedef struct terminal_t {
    uint8_t tid;                        /* terminal index into global terminal array */
    uint32_t curr_x;                    /* every terminal needs to store its x/y coords, how far it is into its own buffer, the buffer itself        */
    uint32_t curr_y;                    /* the video memory buffer, its color, what process is currently active inside it, virtualized rtc variables */
    volatile int buff_idx;              /* and the eflags within it (prevents different terminals from overwriting the eflags register of each other */
    volatile uint8_t buff[BUFF_SIZE];
    uint8_t * vmem;
    uint8_t color;
    uint32_t flags;
    struct process_t * active;
    int term_freq; 
    int term_rtc_counter;
    volatile int term_rtc_flag;  
} terminal_t;

terminal_t terminals[MAX_TERMINALS];    /* global array (data container). Has no purpose accept for storing data upon terminal intialization and terminal usage */
int curr_tid;                           /* tid of the current displayed terminal */

void init_terminals();
uint8_t * create_vmem(uint8_t tid);
int start_terminal(uint8_t tid);
void save_terminal_state();
int swap_terminals(uint8_t tid);

int32_t terminal_read(uint32_t ignore, void * buffer, uint32_t n);
int32_t terminal_write(uint32_t ignore, void * buffer, uint32_t n);
int32_t terminal_open(const uint8_t * filename);
int32_t terminal_close(int32_t fd);

#endif
