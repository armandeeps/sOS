#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "devices/keyboard.h"
#include "filesystem.h"
#include "devices/rtc.h"
#include "terminal.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}

/* Checkpoint 1 tests */

/* The following test various exceptions / interrupts.
 * Each function calls the interrupt through asm.
 * If the asm interrupt call does not call the 
 * correct exception handler, the test returns FAIL.
 * If the handler is called, it should print to screen
 * and while(1).                                        */


/* Exception asserts
 * Description: These functions call the interrupts for each interrupt
 *              through asm.  The int # being called corresponds to 
 *				the specific function's IDT entry.
 * Inputs: None
 * Outputs: None
 * Side Effects: BSOD
 */
static inline void assert_syscall() {
	asm volatile("movl $2, %eax");
	asm volatile("int $0x80");
}

static inline void assert_divide_error()
{
	asm volatile("int $0");
}

static inline void assert_DB_error()
{
	asm volatile("int $1");
}

static inline void assert_nmi()
{
	asm volatile("int $2");
}

static inline void assert_breakpoint()
{
	asm volatile("int $3");
}

static inline void assert_overflow()
{
	asm volatile("int $4");
}

static inline void assert_bound_exceeded()
{
	asm volatile("int $5");
}

static inline void assert_invalid_opcode()
{
	asm volatile("int $6");
}

static inline void assert_device_not_avail()
{
	asm volatile("int $7");
}

static inline void assert_double_fault()
{
	asm volatile("int $8");
}

static inline void assert_seg_overrun()
{
	asm volatile("int $9");
}

static inline void assert_invalid_tss()
{
	asm volatile("int $10");
}

static inline void assert_seg_not_present()
{
	asm volatile("int $11");
}

static inline void assert_sseg_fault()
{
	asm volatile("int $12");
}

static inline void assert_general_protection()
{
	asm volatile("int $13");
}

static inline void assert_page_fault()
{
	asm volatile("int $14");
}

static inline void assert_math_fault()
{
	asm volatile("int $16");
}

static inline void assert_alignment_check()
{
	asm volatile("int $17");
}

static inline void assert_machine_check()
{
	asm volatile("int $18");
}

static inline void assert_FP_exception()
{
	asm volatile("int $19");
}

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

/* Exception asserts
 * Description: These functions call the asm ints and output
                the results using the testing framework.
 * Inputs: None
 * Outputs: None
 * Side Effects: BSOD (except for syscall)
 */

int test_Syscall() 
{
	TEST_HEADER; 
	int result = PASS; 
	assert_syscall(); 
	result = FAIL;
	return result; 
}

int test_DE()
{
	TEST_HEADER;
	int result = PASS;
	assert_divide_error();
	// int x = 0;
	// int y = 10/x;
	// y++; 
	result = FAIL;
	return result;
}

int test_DB()
{
	TEST_HEADER;
	int result = PASS;
	assert_DB_error();
	result = FAIL;
	return result;
}

int test_BP()
{
	TEST_HEADER;
	int result = PASS;
	assert_breakpoint();
	// asm(" INT3; ");
	result = FAIL;
	return result;
}

int test_OF()
{
	TEST_HEADER;
	int result = PASS;
	assert_overflow();
	// asm("mov $0x7f, %%al;" 
	// 	"add $1, %%al;   "
	// 	"INTO;           ");
	result = FAIL;
	return result;
}

int test_BR()
{
	TEST_HEADER;
	int result = PASS;
	assert_bound_exceeded();
	// int a[2] = {0, 0};
	// int a[2] = a[3];
	result = FAIL;
	return result;
}

int test_UD()
{
	TEST_HEADER;
	int result = PASS;
	assert_invalid_opcode();
	// asm("movzzz %%eax, %%eax");
	result = FAIL;
	return result;
}

int test_NM()
{
	TEST_HEADER;
	int result = PASS;
	assert_device_not_avail();
	result = FAIL;
	return result;
}

int test_CS()
{
	TEST_HEADER;
	int result = PASS;
	assert_seg_overrun();
	result = FAIL;
	return result;
}

int test_DF()
{
	TEST_HEADER;
	int result = PASS;
	assert_double_fault();
	result = FAIL;
	return result;
}

int test_TS()
{
	TEST_HEADER;
	int result = PASS;
	assert_invalid_tss();
	result = FAIL;
	return result;
}

int test_NP()
{
	TEST_HEADER;
	int result = PASS;
	assert_seg_not_present();
	result = FAIL;
	return result;
}

int test_SS()
{
	TEST_HEADER;
	int result = PASS;
	assert_sseg_fault();
	result = FAIL;
	return result;
}

int test_GP()
{
	TEST_HEADER;
	int result = PASS;
	assert_general_protection();
	result = FAIL;
	return result;
}

int test_PF() 
{
	TEST_HEADER; 
	int result = PASS;
	assert_page_fault();
	// NULL CASE
	// int * x = NULL;
	// int y = *x; 
	// y++; 
	// EDGE CASE
	// int *x = (int *)0xb7FFF;
	// int y = *x; 
	// y++; 
	result = FAIL; 
	return result; 
}

int test_MF()
{
	TEST_HEADER;
	int result = PASS;
	assert_math_fault();
	result = FAIL;
	return result;
}

int test_AC()
{
	TEST_HEADER;
	int result = PASS;
	assert_alignment_check();
	result = FAIL;
	return result;
}

int test_MC()
{
	TEST_HEADER;
	int result = PASS;
	assert_machine_check();
	result = FAIL;
	return result;
}

int test_XF()
{
	TEST_HEADER;
	int result = PASS;
	assert_FP_exception();
	result = FAIL;
	return result;
}

/* ----------------------------------------------------CHECKPOINT 2 TEST FUNCTIONS-----------------------------------------------------------*/
int test_terminal() {
	uint32_t ignore = 0;
	char * path = "[sOS]$ ";
	unsigned char out[128];
	while(1) {
		terminal_write(ignore, path, 8);
		terminal_read(ignore, out, 128);
		printf("You wrote: %s\n", out); /* shows that terminal read is working */
	}
}

int test_fileSystem(const char *fname, uint32_t index, uint32_t offset, uint8_t byIndex, uint32_t length) {
	dentry_t dentry_obj;
	dentry_t * dentry = &dentry_obj;
	int found = 0;
	int copied = -1;
	uint8_t data[40000]; /* user defined buffer size, size greater than or equal to passed in length */
	int i;
	if (byIndex) {
		found = read_dentry_by_index(index, dentry);
		if(found != -1) {
			copied = read_data(dentry->inode, offset, data, length);
		}
		else {
			printf("File not found!\n");
		}
	}
	else {
		found = read_dentry_by_name((const uint8_t *)fname, dentry);
		if(found != -1) {
			copied = read_data(dentry->inode, offset, data, length);
		}
		else {
			printf("File not found!\n"); 
		}
	}
	if (copied != -1) {
		clear();
		putc('\n');
		for(i = 0; i < copied; i++) {
			if(data[i]) {
				putc(data[i]);
			}
		}
		putc('\n');
	}
	return PASS; 
}

int test_dentry(const char *fname, uint32_t index, uint32_t offset, uint8_t byIndex, uint32_t length){
	dentry_t dentry_obj;
	dentry_t * dentry = &dentry_obj;
	int found = 0;
	//int copied = -1;
	//uint8_t data[10000]; //user defined buffer size, size greater than or equal to passed in length
	//int i;
	if (byIndex) {
		found = read_dentry_by_index(index, dentry);
	if(found != -1){
		printf("FILE NAME: %s  ", dentry->f_name);
		printf("FILE TYPE: %d  ", dentry->f_type);
		//printf("%d  ", dentry->inode.length);
		//printf('\n');

		putc('\n');
		return PASS;
	}
	else{
		printf("File not found!\n"); 
	}
	}

	else{
		found = read_dentry_by_name((const uint8_t *)fname, dentry);
		if(found != -1) {
		printf("FILE NAME: %s  ", dentry->f_name);
		printf("FILE TYPE: %d  ", dentry->f_type);
		//printf("%d  ", dentry->inode->length);
		putc('\n');
		return PASS;
		}
		else {
			printf("File not found!\n"); 
		}
	}

		return PASS;

	}

int test_dir() {
	int i = 0; /* count number of calls made to read_directory */
	while(i < 20) {
		int j = 0;
		char buffer[32];
		int32_t bytes = 32;
		int32_t fd = 0;
		int r = dir_read(fd, buffer, bytes);
		if(r != 0){
			printf("FILE NAME: ");
			for(j = 0; j < r; j++){
				putc(buffer[j]);
			}
			putc('\n');
		}
		// else { /* uncomment to demonstrate that read_directory returns 0 if num of calls exceeds directory entries */
		// 	printf("No files left\n"); /* calls return 0 once no files are left to list in the directory */
		// }
		i++; 
	}
	//read_directory(); /* can change this to be -1 if want to list all files, else change to a specific index for 1 file */
	/* test consecutive calls to read_directory */
	// int index, ret;
	// int stop = 30;
	// for(index = 0;  index < stop; index++) {
	// 	ret = read_directory(index);
	// 	if(ret == 0) {
	// 		break;
	// 	}
	// }
	return PASS;
}

int test_rtc() {
	const char *rtc = "rtc";
	rtc_open((const uint8_t *)rtc); 
	int32_t val = 4; /* change this value to change the start frequency */
	int32_t *freq = &val; 
	int32_t bytes = 4; 
	int32_t z = 75;
	int ret = rtc_write(0, freq, bytes); 
	if(ret != -1) {
		while(1) {
			rtc_read(0, NULL, 4); 
			putc('1');
			z++;
			if(z == 100) {
				z = 0;
				*freq = *freq * 2; /* change the frequency to show rtc_write works */
				 ret = rtc_write(0, freq, bytes); 
				 clear();
				 set_screen_coordinates(0,0);
				 if(ret == -1) {
					 break;
				}
			}
		}
	}
	else {
		printf("Frequency not a power of 2\n"); 
		return FAIL;
	}
	return PASS;
}

/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	//TEST_OUTPUT("idt_test", idt_test());

/* ----------------------------------------------------CHECKPOINT 2 TEST CASES-----------------------------------------------------------*/
	// TEST_OUTPUT("testing terminal driver", test_terminal());

	// TEST_OUTPUT("read_dir", test_dir());
	
    // TEST_OUTPUT("read_dentry", test_dentry("frame0.txt", 1, 5, 1, 8000));
	/* file_name, index, offset, search by index (1 = true, 0 = false), read_length */
	//TEST_OUTPUT("filesystem", test_fileSystem("frame0.txt", 11, 0, 0, 8500));
	//TEST_OUTPUT("filesystem", test_fileSystem("frame1.txt", 11, 0, 0, 8500));
	// TEST_OUTPUT("filesystem", test_fileSystem("verylargetextwithverylongname.tx", 11, 0, 0, 8500));
	//TEST_OUTPUT("filesystem", test_fileSystem("grep", 11, 0, 0, 8500));
	//TEST_OUTPUT("filesystem", test_fileSystem("shell", 2, 0, 1, 8500));
	// TEST_OUTPUT("filesystem", test_fileSystem("cat", 11, 0, 0, 8500));		
	//TEST_OUTPUT("filesystem", test_fileSystem("fish", 0, 0, 0, 8500));
	//TEST_OUTPUT("filesystem", test_fileSystem("pingpong", 8, 0, 1, 8500));

	// TEST_OUTPUT("testing rtc driver", test_rtc());
	
/* ----------------------------------------------------CHECKPOINT 1 TEST CASES-----------------------------------------------------------*/
	// TEST_OUTPUT("testing system call", test_Syscall());
	// TEST_OUTPUT("testing divide error", test_DE());
	// TEST_OUTPUT("testing DB error", test_DB());
	// TEST_OUTPUT("testing break point", test_BP());
	// TEST_OUTPUT("testing overflow", test_OF());
	// TEST_OUTPUT("testing bound error", test_BR());
	// TEST_OUTPUT("testing invalid opcode", test_UD());
	// TEST_OUTPUT("testing device not available", test_NM());
	// TEST_OUTPUT("testing double fault", test_DF());
	// TEST_OUTPUT("testing segment overrun", test_CS());
	// TEST_OUTPUT("testing TSS", test_TS());
	// TEST_OUTPUT("testing Segment not Present", test_NP());
	// TEST_OUTPUT("testing stack segment fault", test_SS());
	// TEST_OUTPUT("testing general protection", test_GP());
	// TEST_OUTPUT("testing page fault", test_PF());
	// TEST_OUTPUT("testing math fault", test_MF());
	// TEST_OUTPUT("testing aligment", test_AC());
	// TEST_OUTPUT("testing machine", test_MC());
	// TEST_OUTPUT("testing floating point", test_XF());
}
