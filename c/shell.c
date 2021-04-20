#include <xeroskernel.h>

extern void ps() {
    struct process_status_list *status_list = kmalloc(sizeof(struct process_status_list));
    sysgetcputimes(status_list);
    print_process_statuses(status_list);
    kfree(status_list);
    return;
}

extern void ex(void) {
    unsigned int shell_id;
    unsigned int any = 0;
    sysrecv(&any, &shell_id);
    syskill(shell_id, 31);
    return;
}

extern void k(void) {
    unsigned int shell_id;
    unsigned int target_id;
    unsigned int any = 0;
    sysrecv(&any, &shell_id);
    sysrecv(&any, &target_id);
    syssend(shell_id, syskill(target_id, 31));
    return;
}

extern void a(void) {
    unsigned int shell_id;
    unsigned int milliseconds;
    unsigned int any = 0;
    sysrecv(&any, &shell_id);
    sysrecv(&any, &milliseconds);
    syssleep(milliseconds);
    syskill(shell_id, 18);
    return;
}



extern void t(void) {
    while (1) {
        sysputs("T\n");
        syssleep(10000);
    }
    return;
}

void alarm(void *context);

void alarm(void *context) {
    sysputs("ALARM ALARM ALARM\n");
    void (*old_handler) (void *);
    syssighandler(18, NULL, &old_handler);
    return;
}

extern void shell() {
    unsigned int shell_id = sysgetpid();
    unsigned long bytes_read;
    unsigned long buffer_length = 255;
    char buffer[256];

    while (1) {

        // syssleep(10000);
        sysputs("> ");
        buffer[0] = '\0';

        unsigned long file_descriptor = sysopen(1);
        bytes_read = sysread(file_descriptor, buffer, buffer_length);
        sysclose(file_descriptor);
        // kprintf("Command Received\n");

        if (bytes_read == 0) {
            break;
        }
        if (bytes_read == -666) {
            continue;
        }

        unsigned long background = 0;

        if (bytes_read == buffer_length) {
            kprintf("ERROR: Command too long\n");
            continue;
        }
        if (buffer[bytes_read-1] != '\n') {
            continue;
        } 
        if (buffer[bytes_read-2] == '&') {
            background = 1;
        }

        buffer[bytes_read-1] = '\0';
        unsigned int command_id;

        char *token = kstrtok(buffer, ' ');

        if (kstrcmp(token, "ps\0") || kstrcmp(token, "ps&\0")) {
            command_id = syscreate(ps, DEFAULT_STACK_SIZE);
            syswait(command_id);

        } else if (kstrcmp(token, "ex\0") || kstrcmp(token, "ex&\0")) {
            command_id = syscreate(ex, DEFAULT_STACK_SIZE);
            syssend(command_id, shell_id);
            syswait(command_id);

        } else if (kstrcmp(token, "k\0")) {
            token = kstrtok(NULL, ' ');
            unsigned int target_id = kstrtoi(token);
            unsigned int result;
            command_id = syscreate(k, DEFAULT_STACK_SIZE);
            syssend(command_id, shell_id);
            syssend(command_id, target_id);
            
            sysrecv(&command_id, &result);
            
            if (result == -514) { 
                kprintf("No such process\n");
            }
            syswait(command_id);

        } else if (kstrcmp(token, "a\0")) {
            token = kstrtok(NULL, ' ');
            unsigned int milliseconds = kstrtoi(token);
            // kprintf("Alarm going in %d ms", milliseconds);
            void (*old_handler) (void *);
            syssighandler(18, alarm, &old_handler);

            command_id = syscreate(a, DEFAULT_STACK_SIZE);
            syssend(command_id, shell_id);
            syssend(command_id, milliseconds);

            if (background == 0) { syswait(command_id); }

        } else if (kstrcmp(token, "t\0")) {
            command_id = syscreate(t, DEFAULT_STACK_SIZE);
            if (background == 0) { syswait(command_id); }

        } else if (kstrcmp(token, "t&\0")) {
            command_id = syscreate(t, DEFAULT_STACK_SIZE);

        } else {
            kprintf("Command not found\n");

        }

    }
    return;
}