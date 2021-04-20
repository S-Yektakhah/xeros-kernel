/* disp.c : dispatcher
 */

#include <i386.h>
#include <xeroskernel.h>
#include <stdarg.h>

// DISPATCHER CUTOFF

extern void pause(unsigned int period) {
    int i = 0;
    for (i = 0; i < period; i++);
    return;
}

extern void dispatchinit() {
    struct pcb *idle_process = create_process(idleproc, 4096, NULL); // Create idle process
    if (idle_process == NULL) {
        kprintf("ERROR: Failed to create root process\n\n");
    } else {
        idle_process->priority = IDLE;
        ready(idle_process);
    }
}

extern void dispatch() {
    struct pcb *process = next(); // selects initial process to run (we need to put it in ready queue)
    struct pcb *new_process;

    va_list *arguments;
    unsigned long target_pid;
    int signal_number;
    unsigned int return_code;

    va_list *device_arguments;
    int device_number;
    int file_descriptor;
    void *buffer;
    int buffer_length;
    int command;

    while (process != NULL) {
        int request_code = contextswitch(process); // Transfers control to process and returns on the next interrupt (IO/trap/fault)
        unsigned long process_pid = process -> pid;
        switch (request_code) {
            case CREATE:
                arguments = get_system_arguments(process); // Extract arguments
                void (*entry_point)(void) = va_arg(*arguments, void (*)(void));
                int stack_size = va_arg(*arguments, int);

                struct pcb *created_process = create_process(entry_point, stack_size, process_pid); // alias the call to dispatcher
                if (created_process == NULL) {
                    kprintf("Error: Failed to create process\n");
                    set_return_value(process, 0); // Set return value to 0 signifying failure ()
                } else {
                    ready(created_process);
                    set_return_value(process, created_process->pid); // Set return value to new process ID
                }
                break;
            case YIELD:
                ready(process); // the requesting process goes back to ready queue
                new_process = next();
                process = new_process; // select the next process
                break;
            case STOP:
                // kprintf("Process %d Stopping\n", process -> pid);
                destroy_process(process); // Deallocate process resources
                // kprintf("Finding next process\n", process -> pid);
                process = next();
                // kprintf("Process %d Resuming\n", process -> pid);
                break;
            case GET_PID:
                set_return_value(process, process_pid);
                break;
            case PUTS:
                arguments = get_system_arguments(process); // Extract arguments
                char *print_statement = va_arg(*arguments, char *);
                kprintf(print_statement);
                break;
            case KILL:
                arguments = get_system_arguments(process); // Extract arguments
                target_pid = va_arg(*arguments, unsigned long);
                signal_number = va_arg(*arguments, int);
                if (target_pid == 0) {
                    set_return_value(process, -514);
                } else {
                    set_return_value(process, signal(signal_number, pcb_lookup(target_pid)));
                }
                break;
            case SET_PRIORITY:
                arguments = get_system_arguments(process); // Extract arguments
                int priority = va_arg(*arguments, int);
                if (priority == -1) {
                    set_return_value(process, process -> priority);
                } else {
                    if (priority < READY_0 || READY_3 < priority) { // Illegal priority values requested
                        set_return_value(process, -1);
                    } else {
                        set_return_value(process, process -> priority);
                        process -> priority = priority;
                    }
                }
                break;
            case SEND:
                arguments = get_system_arguments(process); // Extract arguments
                unsigned long dest_pid = va_arg(*arguments, unsigned long);
                unsigned long message = va_arg(*arguments, unsigned long);
                return_code = send(process, dest_pid, message);
                if (return_code == -1) {
                    set_return_value(process, 0);
                    process = next();
                } else {
                    set_return_value(process, return_code);
                }
                break;
            case RECEIVE:
                arguments = get_system_arguments(process); // Extract arguments
                unsigned long *src_pid_address = va_arg(*arguments, unsigned long *);
                unsigned long *message_address = va_arg(*arguments, unsigned long *);
                return_code = recv(process, src_pid_address, message_address);
                if (return_code == -1) {
                    set_return_value(process, 0);
                    process = next();
                } else {
                    set_return_value(process, return_code);
                }
                break;
            case SLEEP:
                arguments = get_system_arguments(process); // Extract arguments
                unsigned int milliseconds = va_arg(*arguments, unsigned int);
                sleep(process, milliseconds);
                set_return_value(process, 0);
                process = next();
                break;
            case TIMER_INTERRUPT:
                tick(process);
                ready(process); // the requesting process goes back to ready queue
                new_process = next();
                process = new_process; // select the next process
                end_of_intr();
                break;
            case GET_CPU_TIMES:
                arguments = get_system_arguments(process); // Extract arguments
                struct process_status_list *status_list = va_arg(*arguments, struct process_status_list *);
                set_return_value(process, process_statuses(status_list));
                break;
            case SIGNAL_RETURN:
                arguments = get_system_arguments(process); // Extract arguments
                struct context_frame *context = (struct context_frame *) va_arg(*arguments, void *);
                sigreturn(process, context);
                break;
            case SIGNAL_HANDLER:
                arguments = get_system_arguments(process); // Extract arguments
                signal_number = va_arg(*arguments, int);
                void (*new_handler)(void *) = va_arg(*arguments, void (*)(void *));
                void (**old_handler)(void *) = va_arg(*arguments, void (**)(void *));
                set_return_value(process, sighandler(signal_number, process, new_handler, old_handler));                
                break;
            case WAIT:
                arguments = get_system_arguments(process); // Extract arguments
                target_pid = va_arg(*arguments, unsigned long);
                if (target_pid == 0) {
                    set_return_value(process, -1);
                } else {
                    return_code = wait(process, pcb_lookup(target_pid));
                    if (return_code == 0) { process = next(); }
                    set_return_value(process, return_code);
                }
                break;
            case OPEN:
                arguments = get_system_arguments(process); // Extract arguments
                device_number = va_arg(*arguments, int);
                set_return_value(process, di_open(process, device_number));
                break;
            case CLOSE:
                arguments = get_system_arguments(process); // Extract arguments
                file_descriptor = va_arg(*arguments, int);
                set_return_value(process, di_close(process, file_descriptor));
                break;
            case READ:
                arguments = get_system_arguments(process); // Extract arguments
                file_descriptor = va_arg(*arguments, int);
                buffer = va_arg(*arguments, void *);
                buffer_length = va_arg(*arguments, int);

                return_code = di_read(process, file_descriptor, buffer, buffer_length);
                if (return_code == -420) {
                    process = next();
                } else {
                    set_return_value(process, return_code);
                }
                break;
            case WRITE:
                arguments = get_system_arguments(process); // Extract arguments
                file_descriptor = va_arg(*arguments, int);
                buffer = va_arg(*arguments, void *);
                buffer_length = va_arg(*arguments, int);
                set_return_value(process, di_write(process, file_descriptor, buffer, buffer_length));
                break;
            case IOCTL:
                arguments = get_system_arguments(process); // Extract arguments
                file_descriptor = va_arg(*arguments, int);
                command = va_arg(*arguments, unsigned long);
                int device_argument_count = va_arg(*arguments, int);
                device_arguments = va_arg(*arguments, va_list *);
                set_return_value(process, di_ioctl(process, file_descriptor, command, device_argument_count, (va_list *) device_arguments));
                break;
            case KEYBOARD_INTERRUPT:;
                keyboard_tick(get_active_keyboard());
                end_of_intr();
                break;
            default:
                kprintf("ERROR: Unimplemented request code: %d", request_code);
                break;
        }
    }
    kprintf("ERROR: No process to run\n\n");
    return;
}