#ifndef RTC_H
#define RTC_H

#include "../types.h"

#define RTC_IRQ 8
#define RTC_CMD_PORT 0x70
#define RTC_DATA_PORT 0x71

/* MSB turns NMI off, lowest 4 bits hex A, B, & C correspond to 
 * registers A, B, & C respectively.                         */
#define RTC_REG_A 0x8A
#define RTC_REG_B 0x8B
#define RTC_REG_C 0x8C

#define HARDWARE_FREQ 1024
#define HARDWARE_RATE 6
#define DEFAULT_FREQ 2
#define INT_BYTE_SIZE 4
#define SET 1
#define CLEAR 0 

extern void init_rtc();
extern void rtc_handler();
int32_t rtc_open(const uint8_t * filename);
int32_t rtc_close(int32_t fd);
int32_t rtc_read(int32_t fd, void * buf, int32_t nbytes);
int32_t rtc_write(int32_t fd, void * buf, int32_t nbytes);

#endif
