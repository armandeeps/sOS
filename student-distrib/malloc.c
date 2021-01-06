#include "malloc.h"
#include "lib.h"
#include "page.h"
#include "syscalls.h"

void init_memory() {
    mm_node_t *root; 
    mm_node_t *curr; 
    int level = 0; 
    int curr_size = _4MB;
    int children = 1;
    int i; 
    curr = root;
    while (level < MAX_DEPTH) {
        curr->size = curr_size;
        curr->in_use = 0; 
        children *= 2; /* Level i has (i+1)*2 children */
    }
    return; 
}