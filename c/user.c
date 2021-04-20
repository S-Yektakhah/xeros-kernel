/* user.c : User processes
 */

#include <xeroskernel.h>

char *root_username = "cs415\0";
char *root_password = "EveryonegetsanA\0";

extern void root() {
    // unsigned int pid = sysgetpid();
    unsigned int username_correct = 0;
    unsigned int password_correct = 0;
    unsigned long bytes_read;
    unsigned long buffer_length = 255;
    char buffer[255];

    while (1) {
        kprintf("Welcome to Xeros - a not so experimental OS\n");
        buffer[0] = '\0';

        unsigned long file_descriptor = sysopen(1);

        sysputs("Username: ");
        bytes_read = sysread(file_descriptor, buffer, buffer_length-1);
        buffer[bytes_read-1] = '\0';
        username_correct = kstrcmp(buffer, root_username);

        // if (username_correct == 0) { continue; } 

        sysioctl(file_descriptor, 55); // Turn off echoing

        sysputs("Password: ");
        bytes_read = sysread(file_descriptor, buffer, buffer_length-1);
        buffer[bytes_read-1] = '\0';

        password_correct = kstrcmp(buffer, root_password);
        sysputs("\n");

        sysclose(file_descriptor);

        if (username_correct == 1 && password_correct == 1) {
            unsigned long shell_id = syscreate(shell, DEFAULT_STACK_SIZE);
            syswait(shell_id);
        }
    }
    return;
}


extern void producer() {
    return;
}

extern void consumer() {
    return;
}

extern void idleproc() {
    // kprintf("Idle PID: %d\n", sysgetpid());
    for (;;) {
        __asm __volatile("\
            hlt \n\
            "
            :
            :
            : "%eax", "%edx");
    }
    return;
}

unsigned long count;
void counter(void);
void sleeper_a(void);
void sleeper_b(void);
void sleeper_c(void);
void sender(void);
void receiver(void);

void counter() {
    for (count = 0; ; count++) {
        pause(5000);
    }
}

void sleeper_a() {
    syssleep(1000);
    return;
}

void sleeper_b() {
    syssleep(500);
    return;
}

void sleeper_c() {
    syssleep(100);
    return;
}

void sender() {
    // kprintf("Sender PID: %d\n", sysgetpid());
    unsigned int tester_id;
    unsigned int any = 0;
    sysrecv(&any, &tester_id);
    syssend(tester_id, 22);
    syssend(tester_id, 23);
    syssleep(3000);
    // kprintf("Sender Exiting\n");
    return;
}

void receiver() {
    // kprintf("Receiver PID: %d\n", sysgetpid());
    unsigned int message;
    unsigned int any = 0;
    sysrecv(&any, &message);
    syssleep(1000);
    // kprintf("Receive Exiting\n");
    return;
}

void test_suite_2(unsigned int tester_id) {
    // Test syssetprio(priority)

    unsigned int priority = syssetprio(-1);
    if (priority == READY_3) {
        kprintf("Success: Successfully read priority level\n");
    } else {
        kprintf("Failure: Priority level invalid\n");
    }
    priority = syssetprio(READY_1);
    if (priority == READY_3) {
        kprintf("Success: Successfully read priority level\n");
    } else {
        kprintf("Failure: Priority level invalid\n");
    }
    priority = syssetprio(-1);
    if (priority == READY_1) {
        kprintf("Success: Successfully updated priority level\n");
    } else {
        kprintf("Failure: Priority not updated\n");
    }
    priority = syssetprio(999);
    if (priority == -1) {
        kprintf("Success: Invalid priority denied\n");
    } else {
        kprintf("Failure: Invalid priority accepted\n");
    }
    syssetprio(READY_3);

    // Setup for sender receiver tests

    unsigned int sender_id = syscreate(sender, DEFAULT_STACK_SIZE);
    syssend(sender_id, tester_id); // Send to receiving receiver
    unsigned int receiver_id = syscreate(receiver, DEFAULT_STACK_SIZE);
    int ipc_return_code;

    // 1. Two tests associated with sends.

    ipc_return_code = syssend(receiver_id, 21); // Send to receiving receiver
    if (ipc_return_code == 0) {
        kprintf("Success: Successfully sent message from test to receiver\n");
    } else {
        kprintf("Failure: Failed to send message from test to receiver (got %d)\n", ipc_return_code);
    }
    ipc_return_code = syssend(receiver_id, 21); // Send to terminating receiver
    if (ipc_return_code == -1) {
        kprintf("Success: Successfully detected receiver termination\n");
    } else {
        kprintf("Failure: Failed to detect receiver termination (got %d)\n", ipc_return_code);
    }
    ipc_return_code = syssend(receiver_id, 21); // Send to terminated receiver
    if (ipc_return_code == -2) {
        kprintf("Success: Send to stopped process denied\n");
    } else {
        kprintf("Failure: Send to stopped process not denied (got %d)\n", ipc_return_code);
    }

    // 3. One test of a send failure not demonstrated in the producer consumer problem.

    ipc_return_code = syssend(tester_id, 21); // Send to self
    if (ipc_return_code == -3) {
        kprintf("Success: Send to self denied\n");
    } else {
        kprintf("Failure: Send to self not denied (got %d)\n", ipc_return_code);
    }

    // 2. Two tests associated with receive.

    unsigned int original_sender = sender_id;
    unsigned int any_sender = 0;
    unsigned int invalid_pid = -999;
    unsigned int message;

    ipc_return_code = sysrecv(&invalid_pid, &message); // Receive from invalid PID
    if (ipc_return_code == -2) {
        kprintf("Success: Successfully denied a receive from non-existent process\n");
    } else {
        kprintf("Failure: Failed to deny a receive from non-existent process (got %d)\n", ipc_return_code);
    }
    ipc_return_code = sysrecv(&tester_id, &message); // Receive from self
    if (ipc_return_code == -3) {
        kprintf("Success: Successfully denied a receive from self\n");
    } else {
        kprintf("Failure: Failed to deny a receive from self (got %d)\n", ipc_return_code);
    }

    // 4. Two tests of receive failures not demonstrated

    ipc_return_code = sysrecv(&sender_id, (unsigned int *) 0); // Receive with invalid msg address
    if (ipc_return_code == -4) {
        kprintf("Success: Successfully denied a receive to invalid message address\n");
    } else {
        kprintf("Failure: Failed to deny a receive to message address (got %d)\n", ipc_return_code);
    }
    ipc_return_code = sysrecv((unsigned int *) 0, &message); // Receive with invalid sender_pid address
    if (ipc_return_code == -5) {
        kprintf("Success: Successfully denied a receive to invalid sender_pid address\n");
    } else {
        kprintf("Failure: Failed to deny a receive to invalid sender_pid message address (got %d)\n", ipc_return_code);
    }

    ipc_return_code = sysrecv(&sender_id, &message); // Targeted receive from sender
    if (ipc_return_code == 0 && message == 22 && original_sender == sender_id) {
        kprintf("Success: Successfully received specifically from sender\n");
    } else {
        kprintf("Failure: Failed to receive specifically from sender (got %d)\n", ipc_return_code);
    }
    ipc_return_code = sysrecv(&any_sender, &message); // Untargeted receive
    if (ipc_return_code == 0 && message == 23 && original_sender == any_sender) {
        kprintf("Success: Successfully detected message from sender\n");
    } else {
        kprintf("Failure: Failed to detect message from sender (got %d)\n", ipc_return_code);
    }
    any_sender = 0;
    ipc_return_code = sysrecv(&sender_id, &message); // Receive from terminating sender
    if (ipc_return_code == -10) {
        kprintf("Success: Successfully detected sender termination\n");
    } else {
        kprintf("Failure: Failed to detect sender termination (got %d)\n", ipc_return_code);
    }
    ipc_return_code = sysrecv(&any_sender, &message); // Receive in the absence of any other process
    if (ipc_return_code == -10) {
        kprintf("Success: Successfully detected lone process\n");
    } else {
        kprintf("Failure: Failed to detect lone process (got %d)\n", ipc_return_code);
    }

    // 5. One test demonstrating that time-sharing is working.

    unsigned int counter_pid = syscreate(counter, DEFAULT_STACK_SIZE);
    unsigned long before = count;
    pause(10000);
    unsigned long after = count;
    if (before < after) {
        kprintf("Success: Counter process succesfully pre-empted test process\n");
    } else {
        kprintf("Failure: Counter process failed to pre-empt test process\n");
    }

    // 6. Two tests of syskill();

    unsigned int syskill_return_code = syskill(counter_pid, 31);
    if (syskill_return_code == 0) {
        kprintf("Success: Counter process successfully terminated\n");
    } else {
        kprintf("Failure: Failed to terminate counter process\n");
    }

    // Wait a bit for counter process to actually process the signal
    syssleep(100);

    syskill_return_code = syskill(counter_pid, 31);
    if (syskill_return_code == -514) {
        kprintf("Success: Attempt to kill stopped process denied\n");
    } else {
        kprintf("Failure: Attempt to kill stopped process %d not denied. Expected %d, got %d.\n", counter_pid, -514, syskill_return_code);
    }

    syskill_return_code = syskill(-321, 31);
    if (syskill_return_code == -514) {
        kprintf("Success: Attempt to kill invalid PID denied\n");
    } else {
        kprintf("Failure: Attempt to kill invalid PID %d not denied\n", -321);
    }

    // struct process_status_list *status_list  = kmalloc(sizeof(struct process_status_list));
    // sysgetcputimes(status_list);
    // print_process_statuses(status_list);
    // kfree(status_list);

    // Additional Test for Sleep Durations

    // unsigned int pid_a = syscreate(sleeper_a, DEFAULT_STACK_SIZE);
    // unsigned int pid_b = syscreate(sleeper_b, DEFAULT_STACK_SIZE);
    // unsigned int pid_c = syscreate(sleeper_c, DEFAULT_STACK_SIZE);
    // unsigned int pid_d = syscreate(sleeper_b, DEFAULT_STACK_SIZE);
    // kprintf("Expect waking order %d %d %d %d\n", pid_c, pid_d, pid_b, pid_a);
    return;
}

void interrupter(void);
void waitee(void);
void handler_0(void *context);
void handler_1(void *context);
void handler_2(void *context);
void handler_3(void *context);

unsigned long handler_message_index = 0;
unsigned long handler_messages[3];

void interrupter() {
    unsigned int tester_id;
    unsigned int any = 0;
    sysrecv(&any, &tester_id);

    syskill(tester_id, 1);
    syskill(tester_id, 3);
    syskill(tester_id, 2);

    for (;;) {
        syssleep(1000);
        syskill(tester_id, 0);
    }
    return;
}

void waitee() {
    syssleep(1000);
    return;
}

void handler_0(void *context) {
    pause(1000);
    // kprintf("Handler 0\n");
    // handler_message[handler_message_index++] = 0;
}

void handler_1(void *context) {
    pause(1000);
    // kprintf("Handler 1\n");
    handler_messages[handler_message_index++] = 1;
}

void handler_2(void *context) {
    pause(1000);
    // kprintf("Handler 2\n");
    handler_messages[handler_message_index++] = 2;
}

void handler_3(void *context) {
    pause(1000);
    // kprintf("Handler 3\n");
    handler_messages[handler_message_index++] = 3;
}


void test_suite_3(unsigned int tester_id) {
    int return_code;
    unsigned int interrupter_id = syscreate(interrupter, DEFAULT_STACK_SIZE);
    void (*old_handler) (void *);
    return_code = syssighandler(0, handler_0, &old_handler);
    return_code = syssighandler(1, handler_1, &old_handler);
    return_code = syssighandler(2, handler_2, &old_handler);
    return_code = syssighandler(3, handler_3, &old_handler);

    return_code = syssend(interrupter_id, tester_id);
    syssleep(1000);

    // 1. Test showing prioritization and signals interrupting each other.
    if (handler_messages[0] == 3 && handler_messages[1] == 2 && handler_messages[2] == 1) {
        kprintf("Success: Successfully reordered signals by priority\n");
    } else {
        kprintf("Failure: Failed to prioritize signals. Expected [%d, %d, %d], got [%d, %d, %d]\n", 3, 2, 1, handler_messages[0], handler_messages[1], handler_messages[2]);
    }

    // 2. syssighandler() test case
    return_code = syssighandler(0, NULL, &old_handler);
    if (return_code == 0 && old_handler == handler_0) {
        kprintf("Success: Successfully disabled signal 0\n");
    } else {
        kprintf("Failure: Failed to disable signal 0\n");
    }
    return_code = syssighandler(0, handler_0, &old_handler);
    if (return_code == 0 && old_handler == NULL) {
        kprintf("Success: Successfully enabled signal 0\n");
    } else {
        kprintf("Failure: Failed to enable signal 0. Expected %d, %d; got %d, %d\n", 0, NULL, return_code, old_handler);
    }
    return_code = syssighandler(31, handler_0, &old_handler);
    if (return_code == -1) {
        kprintf("Success: Successfully rejected invalid signal number\n");
    } else {
        kprintf("Failure: Failed to reject invalid signal number. Expected %d, got %d\n", -1, return_code);
    }
    syssleep(100);
    if (is_in_memory_space(-10298341)) {
        kprintf("Integrity Error\n");
    }  
    return_code = syssighandler(0, (void (*)(void *)) (-10298341), &old_handler);
    if (return_code == -2) {
        kprintf("Success: Successfully rejected invalid new handler address\n");
    } else {
        kprintf("Failure: Failed to reject invalid new handler address. Expected %d, got %d\n", -2, return_code);
    }
    syssleep(100);
    return_code = syssighandler(0, handler_0, (void (**)(void *)) (-10298341));
    if (return_code == -3) {
        kprintf("Success: Successfully rejected invalid old handler address\n");
    } else {
        kprintf("Failure: Failed to reject invalid old handler address. Expected %d, got %d\n", -3, return_code);
    }

    // 3. syskill() test case
    return_code = syskill(tester_id, 0);
    if (return_code == 0) {
        kprintf("Success: Successfully sent a signal\n");
    } else {
        kprintf("Failure: Failed to send a signal. Expected %d, got %d\n", 0, return_code);
    }
    return_code = syskill(0, 0);
    if (return_code == -514) {
        kprintf("Success: Successfully rejected signal to invalid PID\n");
    } else {
        kprintf("Failure: Failed to reject signal to invalid PID. Expected %d, got %d\n", -514, return_code);
    }
    return_code = syskill(tester_id, 42);
    if (return_code == -583) {
        kprintf("Success: Successfully rejected invalid signal_number\n");
    } else {
        kprintf("Failure: Failed to reject invalid signal number. Expected %d, got %d\n", -583, return_code);
    }

    // 4. syssigwait() test case
    return_code = syswait(tester_id);
    if (return_code == -666) {
        kprintf("Success: Successfully interrupted syswait\n");
    } else {
        kprintf("Failure: Failed to interrupt syswait. Expected %d, got %d\n", -666, return_code);
    }
    unsigned int waitee_id = syscreate(waitee, DEFAULT_STACK_SIZE);

    syssighandler(0, NULL, &old_handler);
    syssleep(10);
    return_code = syswait(waitee_id);
    if (return_code == 0) {
        kprintf("Success: Successfully completed syswait\n");
    } else {
        kprintf("Failure: Failed to completed syswait. Expected %d, got %d\n", 0, return_code);
    }
    syssighandler(0, handler_0, &old_handler);

    return_code = syswait(0);
    if (return_code == -1) {
        kprintf("Success: Successfully rejected syswait for invalid PID\n");
    } else {
        kprintf("Failure: Failed to reject syswait for invalid PID. Expected %d, got %d\n", -1, return_code);
    }

    syssighandler(0, NULL, &old_handler);

    // 5. sysopen() with invalid arguments 
    unsigned int file_descriptor;
    return_code = sysopen(0);
    file_descriptor = return_code;
    if (0 <= return_code && return_code < FILE_DESCRIPTOR_TABLE_SIZE) {
        kprintf("Success: Successfully opened device\n");
    } else {
        kprintf("Failure: Failed to open device\n");
    }
    return_code = sysopen(0);
    if (return_code == -1) {
        kprintf("Success: Successfully rejected opening of open device\n");
    } else {
        kprintf("Failure: Failed to reject opening of open device\n");
    }
    return_code = sysopen(1);
    if (return_code == -1) {
        kprintf("Success: Successfully rejected opening of multiple keyboards\n");
    } else {
        kprintf("Failure: Failed to reject opening of multiple keyboards, current keyboard = %d\n", get_active_keyboard());
    }
    return_code = sysopen(-1);
    if (return_code == -1) {
        kprintf("Success: Successfully rejected opening of invalid device number\n");
    } else {
        kprintf("Failure: Failed to reject opening of invalid device number\n");
    }

    return_code = sysclose(file_descriptor);
    if (return_code == 0) {
        kprintf("Success: Successfully closed device\n");
    } else {
        kprintf("Failure: Failed to close device\n");
    }

    return_code = sysclose(file_descriptor);
    if (return_code == -1) {
        kprintf("Success: Successfully rejected closing of closed device\n");
    } else {
        kprintf("Failure: Failed to reject closing of closed device\n");
    }
    return_code = sysclose(-1);
    if (return_code == -1) {
        kprintf("Success: Successfully rejected closing of invalid file descriptor\n");
    } else {
        kprintf("Failure: Failed to reject closing of invalid file descriptor\n");
    }
    return_code = sysclose(file_descriptor + 1);
    if (return_code == -1) {
        kprintf("Success: Successfully rejected closing of blank file descriptor\n");
    } else {
        kprintf("Failure: Failed to reject closing of blank file descriptor %d\n", file_descriptor);
    }

    file_descriptor = sysopen(0);
    unsigned long buffer_length = 2;
    char buffer[2];
    buffer[buffer_length] = '\0';
    
    // 6. syswrite() with invalid file descriptor
    return_code = syswrite(-1, buffer, buffer_length);
    if (return_code == -1) {
        kprintf("Success: Successfully rejected syswrite to invalid file_descriptor\n");
    } else {
        kprintf("Failure: Failed to reject syswrite to invalid file_descriptor %d\n", -1);
    }

    // 7. sysioctl() test for invalid commands
    return_code = sysioctl(file_descriptor, 22);
    if (return_code == -1) {
        kprintf("Success: Successfully rejected sysioctl with invalid command\n");
    } else {
        kprintf("Failure: Failed to reject sysioctl with invalid command %d\n", 22);
    }

    // 8. sysread() when there are more characters buffered in kernel than the read requests
    struct pcb *process = pcb_lookup(tester_id);
    struct file_descriptor *descriptor = process -> file_descriptors[file_descriptor];
    struct device *keyboard = descriptor -> device;
    int bytes_read;
    read_character(process, keyboard, (unsigned long) 'A'); // Similuate keyboard input
    read_character(process, keyboard, (unsigned long) 'B'); // Similuate keyboard input
    read_character(process, keyboard, (unsigned long) 'C'); // Similuate keyboard input
    read_character(process, keyboard, (unsigned long) 'D'); // Similuate keyboard input

    bytes_read = sysread(file_descriptor, buffer, buffer_length-1);
    if (bytes_read == 1 && buffer[0] == 'A') {
        kprintf("Success: Successfully read first character in kernel buffer\n");
    } else {
        kprintf("Failure: Failed to read first character in kernel buffer. Expected %c, got %c\n", 'A', buffer[0]);
    }

    bytes_read = sysread(file_descriptor, buffer, buffer_length-1);
    if (bytes_read == 1 && buffer[0] == 'B') {
        kprintf("Success: Successfully read second character in kernel buffer\n");
    } else {
        kprintf("Failure: Failed to read second character in kernel buffer. Expected %c, got %c\n", 'B', buffer[0]);
    }

    bytes_read = sysread(file_descriptor, buffer, buffer_length-1);
    if (bytes_read == 1 && buffer[0] == 'C') {
        kprintf("Success: Successfully read third character in kernel buffer\n");
    } else {
        kprintf("Failure: Failed to read third character in kernel buffer. Expected %c, got %c\n", 'C', buffer[0]);
    }

    bytes_read = sysread(file_descriptor, buffer, buffer_length-1);
    if (bytes_read == 1 && buffer[0] == 'D') {
        kprintf("Success: Successfully read last character in kernel buffer\n");
    } else {
        kprintf("Failure: Failed to read last character in kernel buffer. Expected %c, got %c\n", 'D', buffer[0]);
    }

    sysclose(file_descriptor);

    // 9. Two test cases for scenarios not covered here or in the test program.
    // Additional Tests:

    // Sysleep's interrupted return value
    syssighandler(0, handler_0, &old_handler);
    unsigned int milliseconds_remaining = syssleep(1000);
    if (milliseconds_remaining > 0) {
        kprintf("Success: Successfully interrupted syssleep\n");
    } else {
        kprintf("Failure: Failed to interrupt syssleep. Expected > 0, got %d\n", milliseconds_remaining);
    }

    // Sysread's interrupted return value
    file_descriptor = sysopen(0);
    unsigned int read_interruption_return_value = sysread(file_descriptor, buffer, buffer_length-1);
    sysclose(file_descriptor);
    if (read_interruption_return_value == -666) {
        kprintf("Success: Successfully interrupted sysread\n");
    } else {
        kprintf("Failure: Failed to interrupt sysread. Expected %d, got %d\n", -666, read_interruption_return_value); 
    }

    // Send and Receive interruption return value
    unsigned int message;
    return_code = syssend(interrupter_id, 123);
    if (return_code == -666) {
        kprintf("Success: Successfully interrupted syssend\n");
    } else {
        kprintf("Failure: Failed to interrupt syssend. Expected %d, got %d\n", -666, return_code);
    }
    return_code = sysrecv(&interrupter_id, &message);
    if (return_code == -666) {
        kprintf("Success: Successfully interrupted sysrecv\n");
    } else {
        kprintf("Failure: Failed to interrupt sysrecv. Expected %d, got %d\n", -666, return_code);
    }

    syssighandler(0, NULL, &old_handler);
    syskill(interrupter_id, 31);
    return;
}

extern void test_process() {
    unsigned int tester_id = sysgetpid();
    // kprintf("Tester PID: %d\n", tester_id);

    // test_suit_1 is called from kernel space in init() // Tests from A1

    test_suite_2(tester_id); // Tests from A2
    kprintf("Suite 2 complete\n\n");
    pause(100000);
    test_suite_3(tester_id); // Tests for A3

    // kprintf("Test Exiting\n");
    return;
}
