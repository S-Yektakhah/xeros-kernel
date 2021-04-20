/* msg.c : messaging system 
   This file does not need to modified until assignment 2
 */

#include <xeroskernel.h>

/* Your code goes here */

int send_helper(struct pcb* sender, unsigned long dest_pid, unsigned long num) {
    // Store sent values
    sender -> message = num;
    sender -> messenger = dest_pid;

    struct pcb *receiver = pcb_lookup(dest_pid);

    if (receiver -> messenger_state == MSG_RECEIVING) {
        // extract receiver addresses
        unsigned long *message_address = (unsigned long *)(receiver->message);
        unsigned long *messenger_address = (unsigned long *)(receiver->messenger);
        if (*messenger_address == 0 || *messenger_address == sender -> pid) { // If receiver is waiting for sender
            // kprintf("Send: %d -> %d -> %d (Unblocking)\n", sender->pid, num, dest_pid);
            // write sent values to extracted addresses
            *message_address = num;
            *messenger_address = sender->pid;

            // set messenger state
            receiver -> messenger_state = MSG_IDLE;
            // ready receiver
            ready(receiver);
            return 0;
        }
    }

    // If receiver is not waiting for sender

    // kprintf("Send: %d -> %d -> %d (Blocking)\n", sender->pid, num, dest_pid);
    // set messenger state
    sender -> messenger_state = MSG_SENDING;
    // block self
    block(sender);
    // Update sender list
    struct pcb *current_node = receiver -> sender_queue;
    if (current_node == NULL) {
        receiver -> sender_queue = sender;
        return -1;
    }
    while (current_node -> sender_next != NULL) {
        current_node = current_node -> sender_next;
    }
    current_node -> sender_next = sender;
    return -1;
};

extern void unsend(struct pcb *sender) {
    if (sender -> messenger_state == MSG_SENDING) {
        struct pcb *receiver = pcb_lookup(sender -> messenger);
        sender -> message = NULL;
        sender -> messenger = NULL;
        sender -> messenger_state = MSG_IDLE;

        struct pcb *current_node = receiver -> sender_queue;
        if (current_node -> pid == sender -> pid) {
            receiver -> sender_queue = current_node -> sender_next;
            current_node -> sender_next = NULL;
            ready(sender);
            return;
        }
        while (current_node -> sender_next != NULL) {
            struct pcb *next_node = current_node -> sender_next;
            if (next_node -> pid == sender -> pid) {
                current_node -> sender_next = next_node -> sender_next;
                next_node -> sender_next = NULL;
                ready(sender);
                return;
            }
            current_node = next_node;
        }
    }
}

int recv_helper(struct pcb *receiver, unsigned long *src_pid, unsigned long *num) {
    // store receiver addresses
    receiver->message = (unsigned long)num;
    receiver->messenger = (unsigned long)src_pid;

    struct pcb *sender = pcb_lookup(*src_pid);

    // Untargeted Receive
    if (*src_pid == 0) {
        if (receiver -> sender_queue == NULL) { // Blocking ReceiveAny
            // kprintf("Receive: %d -> ? -> %d (Blocking)\n", *src_pid, receiver->pid);
            // set messenger state
            receiver -> messenger_state = MSG_RECEIVING;
            // block self
            block(receiver);
            return -1;
        } else { // Non-blocking ReceiveAny
            // kprintf("Receive: %d -> ? -> %d (Unblocking)\n", *src_pid, receiver->pid);
            // pop fom sender queue
            sender = receiver -> sender_queue;
            receiver -> sender_queue = sender -> sender_next;
            sender -> sender_next = NULL;

            // extract sent values and write to addresses
            *src_pid = sender -> pid;
            *num = sender -> message;

            // set messenger state
            sender -> messenger_state = MSG_IDLE;
            // ready the sender
            ready(sender);
            return 0;
        }
    } else { // Targeted Receive
        if ( (sender -> messenger_state) == MSG_SENDING && (sender -> messenger) == (receiver -> pid) ) {
            // kprintf("Receive: %d -> ? -> %d (Unblocking)\n", *src_pid, receiver->pid);

            // pop fom sender queue
            sender = receiver -> sender_queue;
            receiver -> sender_queue = sender -> sender_next;
            sender -> sender_next = NULL;

            // extract sent values and write to addresses
            *src_pid = sender -> pid;
            *num = sender -> message;

            // set messenger state
            sender -> messenger_state = MSG_IDLE;
            // ready the sender
            ready(sender);
            return 0;
        } else {
            // kprintf("Receive: %d -> ? -> %d (Blocking)\n", *src_pid, receiver->pid);
            // set messenger state
            receiver -> messenger_state = MSG_RECEIVING;
            // block self
            block(receiver);
            return -1;
        }
    }
};

extern void unreceive(struct pcb *receiver) {
    if (receiver -> messenger_state == MSG_RECEIVING) {
        receiver->message = NULL;
        receiver->messenger = NULL;
        receiver -> messenger_state = MSG_IDLE;
        ready(receiver);
    }
    return;
}

extern int send(struct pcb *sender, unsigned long dest_pid, unsigned long num) {
    struct pcb *receiver = pcb_lookup(dest_pid);
    if (receiver == NULL) { return -2; }
    if (receiver -> pid  == sender -> pid) { return -3; }

    return send_helper(sender, dest_pid, num);
}

extern int recv(struct pcb *receiver, unsigned long *src_pid, unsigned long *num) {
    if (process_owns_memory(receiver, (unsigned long) num) == FALSE) { return -4; }
    if (process_owns_memory(receiver, (unsigned long) src_pid) == FALSE) { return -5; }

    if (pcb_alone(receiver) == TRUE) { return -10; }

    struct pcb *sender = pcb_lookup(*src_pid);
    if (sender  == NULL && *src_pid != 0) { return -2; }
    if (sender -> pid  == receiver -> pid) { return -3; }

    return recv_helper(receiver, src_pid, num);
}