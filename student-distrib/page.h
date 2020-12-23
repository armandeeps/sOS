#ifndef PAGE_H
#define PAGE_H

#include "types.h"

#define PDE_SIZE 1024
#define PTE_SIZE 1024
#define PRESENT  0x1
#define PRESENT_PHYSICAL 0x1
#define R_W      0x2
#define User_SUP 0x4
#define PWT      0x8
#define PCD      0x10
#define A        0x20
#define PS       0x80
#define GLOBAL   0x100
#define KERNEL_ADDR 0x400000 
#define VIDEO_MEMORY_START 0xB8000
#define VIRTUAL_VIDEO_START  (VIDEO_MEMORY_START >> 12)
#define FOURKB   4096
#define VIDEO_PTE 0
#define USR_VIDEO_PDE 33 /* User video mapped to 132 MB, 132 / 4MB = 33 */

extern void init_paging();
uint32_t Page_Directory[PDE_SIZE] __attribute__((aligned(4 * PDE_SIZE)));
uint32_t Page_Table[PTE_SIZE] __attribute__((aligned(4 * PTE_SIZE)));

#endif
