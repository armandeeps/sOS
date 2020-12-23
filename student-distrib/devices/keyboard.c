#include "keyboard.h"
#include "../lib.h"
#include "../schedule.h"

/* keyboard_keys associates SHIFT,CTRL,CAPS,ALT states with the printable characters during those states */
static uint8_t keyboard_keys[STATES][NUM_KEYS] = {
    /* no shift no caps */
    {'\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\0', '\0',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\0', '\0', 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l' , ';', '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v', 
    'b', 'n', 'm',',', '.', '/', '\0', '*', '\0', ' ', '\0'}, 
    /* yes shift no cap */
    {'\0', '\0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\0', '\0',
	 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\0', '\0', 'A', 'S',
	 'D', 'F', 'G', 'H', 'J', 'K', 'L' , ':', '"', '~', '\0', '|', 'Z', 'X', 'C', 'V', 
	 'B', 'N', 'M', '<', '>', '?', '\0', '*', '\0', ' ', '\0'},
    /* no shift yes cap*/
    {'\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\0', '\0',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\0', '\0', 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L' , ';', '\'', '`', '\0', '\\', 'Z', 'X', 'C', 'V', 
    'B', 'N', 'M',',', '.', '/', '\0', '*', '\0', ' ', '\0'},
    /* yes shift yes cap */ 
    {'\0', '\0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\0', '\0',
	 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '{', '}', '\0', '\0', 'a', 's',
	 'd', 'f', 'g', 'h', 'j', 'k', 'l' , ':', '"', '~', '\0', '|', 'z', 'x', 'c', 'v', 
	 'b', 'n', 'm', '<', '>', '?', '\0', '*', '\0', ' ', '\0'}
};
/* Used to track internal state of the keyboard presses */
static unsigned char SHIFT = RELEASED; 
static unsigned char CTRL = RELEASED; 
static unsigned char CAPS = RELEASED;
static unsigned char ALT = RELEASED;

/* keyboard_handler()
 * Description: Handle keyboard interrupts by displaying character.
 * Inputs: none
 * Outputs: none
 * Returns: none
 * Side Effects: Modifies global variables used in lib.c/terminal functions
 */
void keyboard_handler() {
    /* Regardless of current_process, keyboard_handler is called on visible terminal 
    *  so swap global video_mem (used in lib.c functions), kb_buff, buff_idx, and make a temp for the current kb_buff
    *  write to visible screen (kb_putc) */
    volatile uint8_t* old_buff = kb_buff;
    video_mem = terminals[curr_tid].vmem;
    kb_buff = terminals[curr_tid].buff;
    buff_idx = terminals[curr_tid].buff_idx;

    uint8_t code = inb(PS2_DATA_PORT);
    switch(code) {
        case L_SHIFT: 
            SHIFT = SHIFT_PRESSED; 
            goto RET;
        case R_SHIFT:
            SHIFT = SHIFT_PRESSED; 
            goto RET; 
        case L_SHIFT_RELEASED:
            SHIFT = RELEASED;
            break; 
        case R_SHIFT_RELEASED:
            SHIFT = RELEASED; 
            break;
        case CAPS_LOCK: 
            if (CAPS) {
                CAPS = RELEASED;
            }
            else {
                CAPS = CAPS_PRESSED;
            }
            goto RET;
        case CTRL_PRESSED:
            CTRL = PRESSED;
            goto RET;
        case CTRL_RELEASED:
            CTRL = RELEASED; 
            goto RET;
        case ALT_PRESSED:
            ALT = PRESSED;
            goto RET;
        case ALT_RELEASED:
            ALT = RELEASED;
            goto RET;
        case F1:
            // if(ALT) {
                /* Swapping to TERM0 should change the global video_mem var so functions in lib.c work properly on visible terminal
                *  store current buff_idx update kb_buff */
                video_mem = terminals[curr_tid].vmem;
                terminals[curr_tid].buff_idx = buff_idx;
                kb_buff = old_buff;
                send_eoi(KEYBOARD_IRQ);
                swap_terminals(TERM0);
                return;
            // }
            break;
        case F2:
            // if(ALT) {
                video_mem = terminals[curr_tid].vmem;
                terminals[curr_tid].buff_idx = buff_idx;
                kb_buff = old_buff;
                send_eoi(KEYBOARD_IRQ);
                swap_terminals(TERM1);
                return;
            // }
            break;
        case F3:
            // if(ALT) {
                video_mem = terminals[curr_tid].vmem;
                terminals[curr_tid].buff_idx = buff_idx;
                kb_buff = old_buff;
                send_eoi(KEYBOARD_IRQ);
                swap_terminals(TERM2);
                return;
            // }
            break;
        case L:
            if(CTRL) {
                /* Clear the screen and set x,y to be upper left */
                clear();
                set_screen_coordinates(0,0); 
                char * buffer = "391OS> ";
                /* reprint the buffer after ctrl+L */
                int i = 0;
                while(buffer[i]!='\0') {
                    kb_putc(buffer[i]);
                    i++;
                }
                i = 0;
                while(kb_buff[i]!='\0') { 
                    kb_putc(kb_buff[i]);
                    i++;
                }
            }
            else {
                break;
            }
            goto RET;
        case ENTER:
            kb_putc('\n');
            kb_buff[buff_idx] = '\n';
            goto RET;
        case BACKSPACE:
            if(buff_idx != 0) {
                int r = screen_backspace();
                if(r != 1){
                    buffer_backspace(); /* only buffer backspace of screen_backspace allows it */
                } 
            }
            goto RET;        
        default:
                break;
    }
    /* parameter/state check before putting character into keyboard_buffer, buff_idx < 127 bc max_char is 128 (including newline) */
    if ((code < NUM_KEYS) && (buff_idx < (BUFF_SIZE - 1)) && !CTRL && (code != F1)) {
        kb_buff[buff_idx] = keyboard_keys[SHIFT + CAPS][code]; 
        buff_idx++; /* update buff_idx to point to next available spot */
        kb_putc(keyboard_keys[SHIFT + CAPS][code]);
    }
    RET:
        video_mem = terminals[curr_tid].vmem;
        terminals[curr_tid].buff_idx = buff_idx;
        kb_buff = old_buff;
        send_eoi(KEYBOARD_IRQ);
}

/* buffer_backspace()
 * Description: Remove the last character in the visible buffer and replace with 
 *              end string char.
 * Inputs: none
 * Outputs: none
 * Returns: none
 * Side Effects: None
 */
void buffer_backspace() {
    buff_idx--; 
    kb_buff[buff_idx] = '\0'; /* decrement buff_idx and clear the kb_buff in that position with a '\0' */
}

/* clear_buffer()
 * Description: Replace n characters in the buffer with the end of string char.
 * Inputs: buffer - address of buffer to clear
 *         n - number of characters to clear
 * Outputs: none
 * Returns: none
 * Side Effects: None
 */
void clear_buffer(unsigned char * buffer, uint32_t n) {
    if(buffer == NULL) {
        return;
    }
    int i;
    for(i = 0; i < n ; i++) {
        buffer[i] = '\0'; /* clear the whole passed in buffer with '\0' */
    }
}

/* init_keyboard()
 * Description: Tells PIC to unmask keyboard interrupt line.
 * Inputs: none
 * Outputs: none
 * Returns: none
 * Side Effects: None
 */
void init_keyboard() {
    enable_irq(KEYBOARD_IRQ);
}

/* disable_keyboard()
 * Description: Tells PIC to mask keyboard interrupt line.
 * Inputs: none
 * Outputs: none
 * Returns: none
 * Side Effects: None
 */
void disable_keyboard() {
    disable_irq(KEYBOARD_IRQ);
}
