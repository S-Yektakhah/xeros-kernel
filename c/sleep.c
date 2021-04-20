/* sleep.c : sleep device 
   This file does not need to modified until assignment 2
 */

#include <xeroskernel.h>
#include <xeroslib.h>

unsigned long ticks = 0;
struct pcb *sleeper_list;

void decrement(void);
void insert(struct pcb *process, unsigned long ticks);

void insert(struct pcb *process, unsigned long ticks) {
    if (ticks <= 0) { return; }
    block(process);
    if (sleeper_list == NULL) {
        sleeper_list = process;
        process->ticks_delayed = ticks;
    } else {
        struct pcb *current_node = sleeper_list;

        if (ticks <= current_node -> ticks_delayed) {
            current_node -> ticks_delayed = current_node -> ticks_delayed - ticks;
            process -> next = current_node;
            sleeper_list = process;
            process -> ticks_delayed = ticks;
            return;
        } else {
            ticks = ticks - (current_node -> ticks_delayed);
        }
        
        while (current_node -> next != NULL) {
            struct pcb *next_node = current_node -> next;
            if (ticks <= next_node -> ticks_delayed) {
                next_node -> ticks_delayed = (next_node -> ticks_delayed) - ticks;
                process -> next = next_node;
                current_node -> next = process;
                process -> ticks_delayed = ticks;
                return;
            } else {
                ticks = ticks - (next_node -> ticks_delayed);
                current_node = next_node;
            }
        }

        current_node -> next = process;
        process -> ticks_delayed = ticks;
    }

    return;
}

void decrement(void) {
    if (sleeper_list == NULL) {
        return;
    }
    sleeper_list -> ticks_delayed = (sleeper_list -> ticks_delayed) - 1;
    struct pcb *current_node = sleeper_list;
    while (current_node -> ticks_delayed == 0) {
        // kprintf("Process %d waking at tick %d\n", current_node -> pid, ticks);
        sleeper_list = current_node -> next;
        current_node -> next = NULL;
        ready(current_node);
        current_node = sleeper_list;
    }

    return;
}

extern unsigned int wake(struct pcb * process) {
    unsigned int ticks_remaining = 0;
    struct pcb *current_node = sleeper_list;
    struct pcb *previous_node = NULL;

    if (sleeper_list == NULL || process -> ticks_delayed == 0) {
        // Process not sleeping
        return -1;
    }

    if (sleeper_list -> pid == process -> pid) {
        struct pcb *next_node = sleeper_list->next;
        unsigned int current_ticks = current_node->ticks_delayed;
        ticks_remaining += current_ticks;

        if (next_node == NULL) {
            sleeper_list = NULL;
        } else {
            next_node -> ticks_delayed += current_ticks;
            sleeper_list = next_node;
        }
        current_node->next = NULL;
        current_node->ticks_delayed = 0;
        ready(current_node);
        return ticks_remaining;
    } else if (sleeper_list -> next != NULL) {
        previous_node = sleeper_list;
        current_node = sleeper_list -> next;
        while (current_node != NULL) {
            unsigned int current_ticks = current_node->ticks_delayed;
            ticks_remaining += current_ticks;
            if (current_node->pid == process->pid)
            {
                struct pcb *next_node = current_node->next;
                previous_node->next = next_node;
                next_node->ticks_delayed += current_node->ticks_delayed;

                current_node->next = NULL;
                current_node->ticks_delayed = 0;
                ready(current_node);
                break;
            }
            previous_node = current_node;
            current_node = current_node->next;
        }
        return ticks_remaining;
    } else {
        // Process not found
        return -2;
    }
}

/* Your code goes here */
extern void sleep(struct pcb *process, unsigned int milliseconds) {
    // Convert this to the minimum number of ticks required
    // Insert the process into sleeper_list positioned by # ticks
    unsigned long period = (unsigned long) ((1.0/TIMER_FREQUENCY) * 1000);
    unsigned long ticks = milliseconds / period + (milliseconds % period != 0);
    insert(process, ticks);
    return;
}

extern void tick(struct pcb *process) {
    // This kernel function is called by the interrupt to signify the passage of a timeslice
    // Use this to update any states of the sleep device
    // Decrease the countdown in sleeper_list, and ready any awakened processes
    process -> cpu_ticks += 1; 
    ticks += 1;
    decrement();
    return;
}

extern unsigned long get_ticks(void) {
    return ticks;
}
