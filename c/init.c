/* initialize.c - initproc */

#include <i386.h>
#include <xeroskernel.h>
#include <xeroslib.h>

extern int entry( void );  /* start of kernel image, use &start    */
extern int end( void );    /* end of kernel image, use &end        */
extern long freemem; 	/* start of free memory (set in i386.c) */
extern char	*maxaddr;	/* max memory address (set in i386.c)	*/

/************************************************************************/
/***   NOTE:				                                          ***/
/***   This is where the system begins after the C environment has    ***/
/***   been established.  Interrupts are initially DISABLED.  The     ***/
/***   interrupt table has been initialized with a default handler    ***/
/************************************************************************/

/*------------------------------------------------------------------------
 *  The init process, this is where it all begins...
 *------------------------------------------------------------------------
 */

 void test_suite_1(void);

void initproc( void ) {
    kprintf("\n\nCPSC 415, 2018W2 32 Bit Xeros -21.0.0 - even before beta Located at: %x to %x\n\n", &entry, &end);

    // char str[1024];
    // int a = sizeof(str);
    // int b = -17;
    //
    // kprintf("Some sample output to illustrate different types of printing\n\n");
    // /* A busy wait to pause things on the screen, Change the value used in the termination condition to control the pause */
    // for (i = 0; i < 3000000; i++);
    // /* Build a string to print) */
    // sprintf(
    //     str,
    //     "This is the number -17 when printed signed %d unsigned %u hex %x and a string %s.\n      Sample printing of 1024 in signed %d, unsigned %u and hex %x.",
    //     b, b, b, "Hello", a, a, a
    // );
    // /* Print the string */
    // kprintf(
    //     "\n\nThe %dstring is: \"%s\"\n\nThe formula is %d + %d = %d.\n\n\n",
    //     a, str, a, b, a + b
    // );
    // for (i = 0; i < 4000000; i++);
    // /* or just on its own */
    // kprintf(str);
    // /* Add your code below this line and before next comment */

    /* Starting initializers */
    kmeminit(); // Initialize the internal state of memory manager
    /* Start internal memory management test suite */
    test_suite_1(); // Test the memory manager
    processinit(); // Creates PCB's
    contextinit(); // Initializes interrupt handling for context switcher
    dispatchinit();
    deviceinit();

    pause(500000); // wait a little bit

    if (BUILD_CODE == PROD_BUILD) {
        // Create root process
        struct pcb *process = create_process(root, DEFAULT_STACK_SIZE, NULL);
        if (process == NULL) {
            kprintf("ERROR: Failed to create root process\n\n");
        } else {
            ready(process);
        }
    } else if (BUILD_CODE == TEST_BUILD) {
        // Create root process
        struct pcb *process = create_process(test_process, DEFAULT_STACK_SIZE, NULL);
        if (process == NULL) {
            kprintf("ERROR: Failed to create test process\n\n");
        } else {
            ready(process);
        }
    }

    // Start dispatcher and idle process

    dispatch();

    /* Add all of your code before this comment and after the previous comment */
    /* This code should never be reached after you are done */
    kprintf("ERROR: Dispatcher terminated.\nEntering infinite loop... (We shouldn't have reached here)\n\n");
    for(;;); /* loop forever */
}

void test_suite_1() {
    // kprintf("Testing memory manager API:\n\n");

    unsigned long memory_level = kmem_level();
    // kprintf("User memory span: [%d, %d], [%d, %d]\nFree memory level: %d bytes\n\n", freemem, HOLESTART, HOLEEND, (unsigned long) maxaddr, memory_level);

    // API calls to be tested : kmalloc(size_t size), kfree(void *ptr)

    // Invalid Deallocations
    if (kfree((void *) (freemem - 1)) != FALSE) {
        kprintf("FAILURE: Should not be able to free a memory address in kernel space\n\n");
    }
    if (kfree((void *) (HOLESTART + 1)) != FALSE) {
        kprintf("FAILURE: Should not be able to free a memory address in BIOS space\n\n");
    }
    if (kfree((void *) (HOLEEND - 1)) != FALSE) {
        kprintf("FAILURE: Should not be able to free a memory address in BIOS space\n\n");
    }
    if (kfree((void *) ((unsigned long) (maxaddr) + 1)) != FALSE) {
        kprintf("FAILURE: Should not be able to free a memory address in beyond user space\n\n");
    }
    if (kfree((void *) (freemem + (16 - (freemem % 16)) + 1)) != FALSE) {
        kprintf("FAILURE: Should not be able to free a misaligned memory address\n\n");
    }

    // Invalid Allocations
    if (kmalloc((size_t) 0) != 0) {
        kprintf("FAILURE: Should not be able to allocative non-positive size\n\n");
    }

    // Valid Allocations
    unsigned long fifteen_bytes = (unsigned long) kmalloc((size_t) 15);
    if (fifteen_bytes == FALSE) { // == 0 means failure
        kprintf("FAILURE: Should be able to allocate 15 bytes\n\n"); // Failing
    }
    if (kmem_level() != memory_level - 32) {
        kprintf("FAILURE: Free memory level should be memory level - 32\n but it is instead %d\n\n", kmem_level()); // Failing
    }
    unsigned long sixteen_bytes = (unsigned long) kmalloc((size_t) 16);
   
    if (sixteen_bytes == FALSE) {  // == 0 means failure
        kprintf("FAILURE: Should be able to allocate 16 bytes\n\n"); // Passing
    }
    if (kmem_level() != memory_level - 64) {
        kprintf("FAILURE: Free memory level should be memory level - 64\n but it is instead %d\n\n", kmem_level()); // Failing
    }

    // Valid Deallocations
    if (kfree((void *) fifteen_bytes) != TRUE) { // == 1 means success
        kprintf("FAILURE: Should be able to free first 15 bytes\n\n");
    }
    if (kmem_level() != memory_level - 32) {
        kprintf("FAILURE: Free memory level should be back at memory level - 32\n but it is instead %d\n\n", kmem_level()); // Failing
    }

    // Invalid Deallocations
    if (kfree((void *) fifteen_bytes) != FALSE) { // == 0 means failure
        kprintf("FAILURE: Should not be able to free first 15 bytes after it is already free\n\n");
    }

    // Valid Deallocations
    if (kfree((void *) sixteen_bytes) != TRUE) { // == 1 means success
        kprintf("FAILURE: Should be able to free next 16 bytes\n\n"); // Failing
    }
    if (kmem_level() != memory_level) {
        kprintf("FAILURE: Free memory level should be back at memory level\n but it is instead %d\n\n", kmem_level()); // Failing
    }

    unsigned long half_size = memory_level / 2;
    unsigned long aligned_half_size = half_size - (half_size % 16);
    unsigned long other_aligned_half_size = aligned_half_size + 16;
    unsigned long half = (unsigned long) kmalloc((size_t) aligned_half_size);
    unsigned long other_half = (unsigned long) kmalloc((size_t) other_aligned_half_size);
    
    // Valid Allocations
    if (half == FALSE) { // == 0 means failure
        kprintf("FAILURE: Should be able to allocate half of memory\n\n"); // Passing
    }
    // Invalid Allocations
    if (other_half != FALSE) { // == 0 means failure
        kprintf("FAILURE: Should not be able to allocate other half of memory due ot BIOS partitioning causing other half to be too small\n\n"); // Passing
    }
    if (kmem_level() != memory_level - (16 + aligned_half_size)) {
        kprintf("FAILURE: Free memory level should be memory_level - header_size - aligned_half_size\n but it is instead %d\n\n", kmem_level()); // Failing
    }

    // Valid Deallocations
    if (kfree((void *) half) != TRUE) { // == 1 means success
        kprintf("FAILURE: Should be able to free half of memory\n\n"); // Looping
    }
    if (kmem_level() != memory_level) {
        kprintf("FAILURE: Free memory level should be at original memory_level\n but it is instead %d\n\n", kmem_level());
    }

    // kprintf("Memory manager test complete\n\n");
    pause(500000);
}
