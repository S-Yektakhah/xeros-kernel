/* syscall.c : syscalls
 */

#include <xeroskernel.h>
#include <stdarg.h>

unsigned int call_code;
va_list arguments;
va_list *arguments_address = &arguments;
va_list device_arguments;
va_list *device_arguments_address = &device_arguments;
int return_value;

// 0 signals failure, since PID 0 is inaccessible by design (it starts at 32)
extern unsigned int syscreate(void (*process_entry_point)(void), int stack) {
    return syscall(CREATE, 2, process_entry_point, stack);
}
extern void sysyield() {
    syscall(YIELD, 0);
    return;
}
extern void sysstop() {
    syscall(STOP, 0);
    return; 
};

extern PID_t sysgetpid() {
    return syscall(GET_PID, 0);
}

extern void sysputs(char *str) {
    syscall(PUTS, 1, str);
}

extern int syskill(PID_t pid, int signal_number) {
    return syscall(KILL, 2, (unsigned long) pid, signal_number);
}

extern int syssetprio(int priority) {
    return syscall(SET_PRIORITY, 1, priority);
}

extern int syssend(unsigned int dest_pid, unsigned long num) {
    return syscall(SEND, 2, (unsigned long) dest_pid, num);
}

extern int sysrecv(unsigned int *src_pid, unsigned int *num) {
    return syscall(RECEIVE, 2, (unsigned long *) src_pid, (unsigned long)num);
}

extern unsigned int syssleep(unsigned int milliseconds) {
    return syscall(SLEEP, 1, milliseconds);
}

extern int sysgetcputimes(struct process_status_list *status_list) {
    return syscall(GET_CPU_TIMES, 1, status_list);
}

extern void syssigreturn(void *context) {
    syscall(SIGNAL_RETURN, 1, context);
    return;
}

extern int syssighandler(int signal_number, void (*new_handler)(void *), void (**old_handler)(void *)) {
    return syscall(SIGNAL_HANDLER, 3, signal_number, new_handler, old_handler);
}

extern int syswait(PID_t pid) {
    return syscall(WAIT, 1, (unsigned long) pid);
}

extern int sysopen(int device_number) {
    return syscall(OPEN, 1, device_number);
}

extern int sysclose(int file_descriptor) {
    return syscall(CLOSE, 1, file_descriptor);
}

extern int syswrite(int file_descriptor, void *buffer, int buffer_length) {
    return syscall(WRITE, 3, file_descriptor, buffer, buffer_length);
}

extern int sysread(int file_descriptor, void *buffer, int buffer_length) {
    return syscall(READ, 3, file_descriptor, buffer, buffer_length);
}

extern int sysioctl(int file_descriptor, unsigned long command, ...) {
    unsigned long original_command = command;
    // unsigned int size;
    switch (command) {
        case 53:
            command = 1;
            break;
        case 55:
        case 56:
            command = 0;
            break;
        // default:
            // kprintf("ERROR: Unsupported command number in sysioctl\n");
    }
    va_start(device_arguments, command);

    int return_value = syscall(IOCTL, 4, file_descriptor, original_command, command, device_arguments_address);
    va_end(device_arguments);
    return return_value;
}

extern int syscall(int call, int size, ...) {
    call_code = call;
    va_start(arguments, size);
    __asm __volatile("\
        movl call_code, %%eax \n\
        movl arguments_address, %%edx \n\
        int $49 \n\
        mov %%eax, return_value \n\
        "
        :
        :
        : "%eax", "%edx");
    va_end(arguments);
    return return_value;
}