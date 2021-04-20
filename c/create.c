/* create.c : create a process
 */

#include <i386.h>
#include <xeroskernel.h>
#include <stdarg.h>

extern long freemem; // Start of memory space
extern char	*maxaddr; // End of memory space

struct pcb pcb_table[PCB_TABLE_SIZE];
struct pcb* priority_table[PRIORITY_TABLE_SIZE];
struct pcb *remove(struct pcb *targetPCB);

// Internal API
struct pcb *pop(struct pcb **queue_address);
void push(struct pcb **queue, struct pcb *newPCB);

struct pcb *pop(struct pcb **queue_address) {
    struct pcb *queue = *queue_address;
    struct pcb *next_pcb = queue;
    if (next_pcb == NULL) {
        return NULL;
    } else {
        *queue_address = queue->next;
        next_pcb->next = NULL; // Remove this PCB from the queue
        return next_pcb;
    }
}

void push(struct pcb **queue_address, struct pcb *newPCB) {
    struct pcb *queue = *queue_address;
    if (queue == NULL) {
        *queue_address = newPCB;
        return;
    }
    while (queue->next != NULL) {
        queue = queue -> next;
    }
    queue -> next = newPCB; // Should put newPCB to the back of the queue.
    return;
}

int validate_stack_size(int stack_size);
struct context_frame *initialize_context(unsigned long stack_pointer, unsigned long instruction_pointer);

int validate_stack_size(int stack_size) {
    return stack_size >= sizeof(struct context_frame) ? 1 : 0;
}

struct context_frame *initialize_context(unsigned long stack_pointer, unsigned long instruction_pointer) {
    struct context_frame *context = (struct context_frame *)(stack_pointer); // Create context frame overlay
    context -> edi = 0;
    context -> esi = 0;
    context -> ebp = 0;
    context -> esp = (unsigned long) stack_pointer;
    context -> ebx = 0;
    context -> edx = 0;
    context -> ecx = 0;
    context -> eax = 0;
    context -> iret_eip = instruction_pointer;
    context -> iret_cs = getCS();
    context -> eflags = 0x00003200;
    context -> return_address = (unsigned long) sysstop;
    return context;
}

// External API

extern int pcb_alone(struct pcb *process) {
    unsigned int i;
    for (i = 0; i < PCB_TABLE_SIZE;  i++) {
        struct pcb *current_process = &pcb_table[i];
        if (current_process -> state != STOPPED && current_process -> state != IDLE && current_process -> pid != process -> pid) {
            return FALSE;
        }
    }
    return TRUE;
}

extern struct pcb *pcb_lookup(unsigned long pid) {
    unsigned int table_index = pid % PCB_TABLE_SIZE;
    if (pid < 0 || table_index < 0 || PCB_TABLE_SIZE <= table_index) {
        return NULL;
    } 
    struct pcb *process = &pcb_table[table_index];
    if (process -> pid == pid && process -> state != STOPPED && process -> state != IDLE) {
        return process;
    } else {
        return NULL;
    }
}

extern struct pcb *next() {
    struct pcb *process;
    int i;
    for (i = 0; i <= MAX_READY_PRIORITY; i++) {
        process = pop(&priority_table[i]);
        if (process != NULL) {
            process->state = RUNNING;
            break;
        }
    }
    return process;
}
extern struct pcb *stopped() {
    struct pcb *process = pop(&priority_table[STOPPED]);
    process->state = RUNNING;
    return process;
}

extern void unblock(struct pcb *process) {
    // Perform Unblocking
    if (process -> state == BLOCKED) {
        if (process -> ticks_delayed != 0) {
            unsigned int ticks_remaining = wake(process);
            if (ticks_remaining < 0) {
                kprintf("ERROR: Attempt to wake process %d that is not sleeping", process -> pid);
            }
            set_return_value(process, ticks_remaining * 1000 / TIMER_FREQUENCY);
        } else if (process -> messenger_state ==  MSG_SENDING) {
            unsend(process);
            set_return_value(process, -666);
        } else if (process -> messenger_state ==  MSG_RECEIVING) {
            unreceive(process);
            set_return_value(process, -666);
        } else if (process -> wait_target != NULL) {
            unwait(process);
            set_return_value(process, -666);        
        } else if (process -> input_buffer != NULL) {
            unread(process);
            if (process -> input_buffer_current_length == 0) {
                set_return_value(process, -666);
            } else {
                set_return_value(process, process -> input_buffer_current_length); 
            }
        }
    }
}

extern void ready(struct pcb *process) {
    push(&priority_table[process->priority], process);
    process->state = process->priority;

    return;
}

extern void stop(struct pcb *process) {
    push(&priority_table[STOPPED], process);
    process->state = STOPPED;
    return;
}

extern void block(struct pcb *process) {
    remove(process);
    process->state = BLOCKED;
    return;
}

extern struct pcb *remove(struct pcb *targetPCB) {
    if (PRIORITY_TABLE_SIZE <= targetPCB -> state) { return NULL; }
    struct pcb **queue_address = &priority_table[targetPCB->state];
    struct pcb *queue = *queue_address;
    if (queue == NULL) {
        return NULL; // empty priority
    } else {
        struct pcb *current_node = queue;
        if (current_node -> pid == targetPCB -> pid) {
            *queue_address = queue->next;
            targetPCB -> next = NULL;
            return targetPCB;
        }
        while (current_node->next != NULL) {
            if ((current_node->next)->pid == targetPCB->pid) {
                struct pcb *next = current_node->next;
                current_node->next = next->next;
                next -> next = NULL;
                return targetPCB;
            }
            current_node = current_node->next;
        }
        return NULL; // process not found
    }
}

extern void processinit() {
    for (int i = 0; i < PRIORITY_TABLE_SIZE; i++){
        priority_table[i] = NULL;
    }
    priority_table[STOPPED] = &pcb_table[0];
    for (int i = 0; i < PCB_TABLE_SIZE; i++) {
        // Chains together all PCB Table entries in stoppedQueue
        pcb_table[i].index = i;
        pcb_table[i].pid = NULL;
        pcb_table[i].parent_id = 0;
        pcb_table[i].state = STOPPED;
        pcb_table[i].priority = NULL;
        pcb_table[i].stack_pointer = NULL;
        pcb_table[i].ticks_delayed = 0;
        if (i == PCB_TABLE_SIZE - 1) {
            pcb_table[i].next = NULL;
        } else {
            pcb_table[i].next = &pcb_table[i + 1];
        }
    }
}

extern int create(void (*entry_point)(void), int stack_size) {
    struct pcb *process = create_process(entry_point, (unsigned long) stack_size, NULL);
    if (process == NULL) {
        return 0;
    } else {
        ready(process);
        return 1;
    }
}

extern int process_owns_memory(struct pcb *process, unsigned long address) {
    return ( process -> process_memory <= address ) && ( address < process -> process_memory_end );
}

extern struct pcb *create_process(void (*entry_point)(void), unsigned long stack_size, unsigned long parent_id) {
    if (validate_stack_size(stack_size) == 0) {
        return NULL;
    }
    unsigned long padded_size = stack_size + SAFETY_MARGIN;

    struct pcb *new_pcb = stopped(); // Allocate new pcb
    if (new_pcb == NULL) {
        return NULL;
    }                                                    // No PCB available
    void *process_memory = kmalloc((size_t)padded_size); // Allocate memory for process stack
    if (process_memory == NULL) {
        kprintf("WARNING: Failed to allocate memory for new process\n");
        return NULL;
    } // Not enough memory available

    // Initialize resources
    unsigned long stack_pointer = (unsigned long)(process_memory) + stack_size - sizeof(struct context_frame);

    initialize_context((unsigned long)stack_pointer, (unsigned long)entry_point);

    unsigned long next_pid = (new_pcb->pid) + PCB_TABLE_SIZE;
    if (new_pcb->pid == NULL || next_pid >= PID_MAX) {
        new_pcb->pid = new_pcb->index;
    } else {
        new_pcb->pid = next_pid;
    }
    new_pcb->parent_id = parent_id;
    new_pcb->state = READY_3;
    new_pcb->priority = READY_3;
    new_pcb->stack_pointer = stack_pointer;
    new_pcb->process_memory = (unsigned long)process_memory;
    new_pcb->process_memory_end = (unsigned long)(process_memory) + padded_size;

    new_pcb->ticks_delayed = NULL;

    new_pcb->sender_queue = NULL;
    new_pcb->sender_next = NULL;
    new_pcb->messenger_state = MSG_IDLE;
    new_pcb->messenger = NULL;
    new_pcb->message = NULL;

    new_pcb->cpu_ticks = 0;

    int i;
    for (i = 0; i < SIGNAL_TABLE_SIZE; i++) {
        new_pcb -> signal_handlers[i] = NULL;
    }
    new_pcb -> signal_handlers[SIGNAL_TABLE_SIZE - 1] = (unsigned long) terminate;
    new_pcb -> running_signals = 0;
    new_pcb -> pending_signals = 0;

    new_pcb -> wait_target = NULL;

    for (i = 0; i < FILE_DESCRIPTOR_TABLE_SIZE; i++) {
        new_pcb -> file_descriptors[i] = NULL;
    }

    new_pcb -> input_buffer = NULL;
    new_pcb -> input_buffer_current_length = NULL;
    new_pcb -> input_buffer_max_length = NULL;

    return new_pcb;
}

extern int destroy_process(struct pcb *process) {
    remove(process);

    // for each other live process
    // purge their sender list of messages from this process
    // If they are receiving from this process then unblock with return -1
    // If they are sending to this process then unblock with return -1
    // If there's only one other process and it is sending/receiving then unblock with return -10

    // kprintf("Cleaning up messaging states part 1\n");
    if (process -> messenger_state == MSG_SENDING) {
        struct pcb *receiver = pcb_lookup(process -> messenger);
        struct pcb *sender = receiver -> sender_queue;
        if (sender -> pid == process -> pid) {
            receiver -> sender_queue = sender -> sender_next;
        } else {
            while (sender != NULL) {
                struct pcb *next = sender -> sender_next;
                if (next != NULL && next -> pid == process -> pid) {
                    sender -> sender_next = next -> sender_next;
                }
                sender = next;
            }
        }
    }

    // kprintf("Cleaning up messaging states part 2\n");
    unsigned int process_count = 0;
    struct pcb *last_process;
    unsigned int i;
    for (i = 0; i < PCB_TABLE_SIZE; i++) {
        struct pcb *other_process = &pcb_table[i];
        if (other_process->state != STOPPED && other_process->state != IDLE && other_process->pid != process->pid) {
            // Iterates to all other non-stopped, non-idle, non-self processes
            process_count += 1;
            if (other_process -> messenger_state == MSG_SENDING && other_process -> messenger == process -> pid) {
                last_process = other_process;
                // unblock this sender
                unsend(other_process);
                set_return_value(other_process, -1);
            }
            if (other_process -> messenger_state == MSG_RECEIVING && *((unsigned long *)(other_process -> messenger)) == process -> pid) {
                last_process = other_process;
                // unblock this receiver
                unreceive(other_process);
                set_return_value(other_process, -1);
            }
            if (other_process -> wait_target == process) {
                // unblock this waiter
                unwait(other_process);
                set_return_value(other_process, 0);
            }
        }
    }

    // kprintf("Last Process: %d, Count: %d, Messenger State: %d\n", last_process -> pid, process_count, messenger_state);
    if (process_count == 1) {
        set_return_value(last_process, -10);
    }
    
    process->sender_queue = NULL;
    process->sender_next = NULL;
    process->messenger_state = MSG_IDLE;
    process->messenger = NULL;
    process->message = NULL;

    // kprintf("Deallocating Process Memory\n");
    int free_result = kfree((void *)process->process_memory);
    if (free_result == 0) {
        kprintf("ERROR: Failed to deallocate process memory for PID: %d\n", process->pid);
        return 0;
    }

    // kprintf("Cleaning up memory state\n");
    process->parent_id = NULL;
    process->state = STOPPED;
    process->priority = NULL;
    process->stack_pointer = NULL;
    process->process_memory = NULL;
    process->process_memory_end = NULL;

    // kprintf("Cleaning up sleep state\n");
    process->ticks_delayed = NULL;

    // kprintf("Cleaning up messaging state part 3\n");
    process->sender_queue = NULL;
    process->sender_next = NULL;
    process->messenger_state = MSG_IDLE;
    process->messenger = NULL;
    process->message = NULL;

    // kprintf("Cleaning up cpu time state\n");
    process->cpu_ticks = 0;

    // kprintf("Cleaning up signal state\n");
    for (i = 0; i < SIGNAL_TABLE_SIZE; i++) {
        process -> signal_handlers[i] = NULL;
    }
    process -> signal_handlers[SIGNAL_TABLE_SIZE - 1] = (unsigned long) terminate;
    process -> running_signals = 0;
    process -> pending_signals = 0;

    // kprintf("Cleaning up waiting state\n");
    process -> wait_target = NULL;

    // kprintf("Cleaning up file descriptor state\n");
    for (i = 0; i < FILE_DESCRIPTOR_TABLE_SIZE; i++) {
        if (process -> file_descriptors[i] != NULL) {
            di_unbind(process, i);
        };
        process -> file_descriptors[i] = NULL;
    }

    // kprintf("Cleaning up sysread state\n");
    process -> input_buffer = NULL;
    process -> input_buffer_current_length = NULL;
    process -> input_buffer_max_length = NULL;

    // kprintf("Stopping process\n");
    stop(process);
    return 1;
}

extern int process_statuses(struct process_status_list *status_list) {
    if (!((unsigned long) status_list <= HOLESTART - sizeof(struct process_status_list) || HOLEEND <= (unsigned long) status_list)) {
        return -1;
    } else if (!(freemem <= (unsigned long) status_list && (unsigned long) status_list <= (unsigned long) maxaddr - sizeof(struct process_status_list))) {
        return -2;
    } else {
        int i;
        int process_count = 0;
        for (i = 0; i < PCB_TABLE_SIZE; i++) {
            struct pcb *current_process = &pcb_table[i];
            if (current_process -> state != STOPPED) {
                status_list -> pid[process_count] = current_process -> pid;
                status_list -> status[process_count] = current_process -> state;
                status_list -> cpuTime[process_count] = (current_process -> cpu_ticks) * 1000 / TIMER_FREQUENCY;
                process_count += 1;
            }
        }
        status_list->length = process_count;
        return process_count;
    }
}

extern void print_process_statuses(struct process_status_list *status_list) {
    int length = status_list -> length;
    kprintf("Process Count : %d\n", length);
    kprintf("PID    Status    CPU Time\n");
    for (int j = 0; j < length; j++) {
        char *status_string = NULL;
        unsigned long status_code = status_list -> status[j];
        if (0 <= status_code && status_code <= 3) {
            status_string = "Ready";
        } else if (status_code == 4) {
            status_string = "Idle";
        } else if (status_code == 5) {
            status_string = "Stopped";
        } else if (status_code == 6) {
            status_string = "Running";
        } else if (status_code == 7) {
            struct pcb *process = pcb_lookup(status_list -> pid[j]);
            if (process -> ticks_delayed != 0) {
                status_string = "Sleeping";
            } else if (process -> messenger_state ==  MSG_SENDING) {
                status_string = "Sending";
            } else if (process -> messenger_state ==  MSG_RECEIVING) {
                status_string = "Receiving";
            } else if (process -> wait_target != NULL) {
                status_string = "Waiting"; 
            } else if (process -> input_buffer != NULL) {
                status_string = "Reading";
            }
        }
        kprintf(" %-6d%-10s%d\n", status_list -> pid[j], status_string, status_list -> cpuTime[j]);
    }
    return;
}

extern void process_print()
{
    kprintf("Ready Queue:\n");
    struct pcb *current_node = priority_table[READY_3];
    unsigned int queue_position = 0;
    while (current_node != NULL)
    {
        kprintf("Queue: Ready, Pos: %d, PID: %d, PPID: %d, State: %d, Stack: %d, Next: %d\n",
                queue_position,
                current_node->pid,
                current_node->parent_id,
                current_node->state,
                current_node->stack_pointer,
                (unsigned long)(current_node->next));
        current_node = current_node->next;
        queue_position += 1;
        pause(500000);
    }
    return;
}

extern void set_return_value(struct pcb *process, unsigned long return_value) {
    ((struct context_frame *)(process->stack_pointer))->eax = return_value;
}

extern va_list *get_system_arguments(struct pcb *process) {
    return (va_list *)(((struct context_frame *)(process->stack_pointer))->edx);
}