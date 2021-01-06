#ifndef MALLOC_H
#define MALLOC_H

#include "types.h"

#define MAX_DEPTH 12 

typedef struct mm_node_t {
    uint8_t *addr; 
    uint32_t size; 
    uint8_t in_use;
    mm_node_t *left;
    mm_node_t *right;     
} mm_node_t; 

// typedef struct mm_region_t {
//     uint8_t *addr; 
//     mm_region_t *prev;
//     mm_region_t *next; 
//     uint8_t in_use; 
//     uint32_t size; 
// } mm_region_t;

// uint8_t *mm_start;
void init_memory(); 
void *kmalloc(uint32_t); 
void kfree(void *);

#endif