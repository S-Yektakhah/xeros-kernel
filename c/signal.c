/* signal.c - support for signal handling
   This file is not used until Assignment 3
 */

#include <i386.h>
#include <xeroskernel.h>
#include <xeroslib.h>

extern long freemem; // Start of memory space
extern char	*maxaddr; // End of memory space

/****************/
/* KERNEL SPACE */
/****************/

extern void unwait(struct pcb *process) {
    process -> wait_target = NULL;
    ready(process);
    return;
}

extern int wait(struct pcb *process, struct pcb *target) {
    if (target == NULL) {
        return -1;
    } else {
        process -> wait_target = target;
        block(process);
        return 0;
    }
}

extern int sighandler(int signal_number, struct pcb * process, void (*new_handler)(void *), void (** old_handler)(void *)) {
    if (signal_number < 0 || signal_number > 30) {
        return -1;
    } else if   (!(
                    (0 <= (unsigned long) (new_handler) && (unsigned long) (new_handler) <= HOLESTART)
                    ||
                    (HOLEEND <= (unsigned long) (new_handler) && (unsigned long) (new_handler) <= (unsigned long) maxaddr)
                )) {
        // We can't really declare functions inside process memory
        return -2;
    } else if (!process_owns_memory(process, (unsigned long) old_handler)) {
        return -3;
    } else {
        *old_handler = (void (*)(void *)) (process -> signal_handlers[signal_number]);
        process -> signal_handlers[signal_number] = (unsigned long) new_handler;
        return 0;
    }
}

extern void sigreturn(struct pcb *process, struct context_frame *context) {
    int signal_number;
    unsigned long signal_mask;
    for (signal_number = SIGNAL_TABLE_SIZE - 1;  signal_number >= 0; signal_number--) {
        signal_mask = 1 << signal_number;
        if (((process -> running_signals) & signal_mask) != 0) {
            break;
        } else if (((process -> pending_signals) & signal_mask) != 0) {
            kprintf("ERROR: Attempt to sigreturn from pending signal\n");
            return;
        }
    }
    if (signal_number < 0) {
        // No signal to found?
        kprintf("ERROR: Attempt to sigreturn from unknown signal\n");
        return;
    }

    // Should be able to subtract this mast to unset the signal flag
    process -> running_signals -= signal_mask;
    process -> stack_pointer = (unsigned long) context;

    // kprintf("Sigreturn complete, returning to shell EIP %d, shell ret_addr %d, shell fn %d, root fn %d, systop fn %d, terminate fn %d\n", context -> iret_eip, context -> return_address, (unsigned long) shell, (unsigned long) root, (unsigned long) sysstop, (unsigned long) terminate);
    return;
}

extern void deliver(struct pcb *process) {
    int signal_number;
    for (signal_number = SIGNAL_TABLE_SIZE - 1;  signal_number >= 0; signal_number--) {
        int signal_mask = 1 << signal_number;
        if (((process -> running_signals) & signal_mask) != 0) {
            // Restore current context to finish off running signal handler
            return;
        } else if (((process -> pending_signals) & signal_mask) != 0 && process -> signal_handlers[signal_number] != NULL) {
            // Set up the stack for a new signal handler
            struct trampoline_arguments *arguments = (struct trampoline_arguments *) (process->stack_pointer - sizeof(struct trampoline_arguments));
            struct context_frame *context = (struct context_frame *) (process->stack_pointer - sizeof(struct trampoline_arguments) - sizeof(struct context_frame));

            context -> edi = 0;
            context -> esi = 0;
            context -> ebp = 0;
            context -> esp = (unsigned long) context;
            context -> ebx = 0;
            context -> edx = 0;
            context -> ecx = 0;
            context -> eax = 0;
            context -> iret_eip = (unsigned long) sigtramp;
            context -> iret_cs = getCS();
            context -> eflags = 0x00003200;
            context -> return_address = 0; // ((struct context_frame *)(process -> stack_pointer)) -> return_address;

            arguments -> handler = process -> signal_handlers[signal_number];
            arguments -> context = process -> stack_pointer;

            process -> stack_pointer = (unsigned long) context;

            // Set the signal flags
            process -> running_signals += signal_mask; // Add signal to 'running'
            process -> pending_signals -= signal_mask; // Remove signal from 'pending'
            return;
        }
    }

    // No signal to deliver / process
    return;
}

extern int signal(int signal_number, struct pcb *process) {
    if (signal_number < 0 || 31 < signal_number) {
        return -583;
    } else if (process == NULL) {
        return -514;
    } else if (process -> signal_handlers[signal_number] != NULL) {
        unsigned long signal_mask = 1 << signal_number;
        unblock(process);
        process -> pending_signals = process -> pending_signals | signal_mask;
        return 0;
    } else {
        return 0;
    }
}

/**************/
/* USER SPACE */
/**************/

extern void sigtramp(void (*handler)(void *), void *context) {
    handler(context);
    // kprintf("Calling syssigreturn\n");
    syssigreturn(context);
    kprintf("ERROR: Unreachable code in sigtramp reached\n");
    return;
}

extern void terminate(void *context) {
    sysstop();
    return;
}