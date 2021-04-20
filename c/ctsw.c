/* ctsw.c : context switcher
 */

#include <i386.h>
#include <xeroskernel.h>

/* Your code goes here - You will need to write some assembly code. You must
   use the gnu conventions for specifying the instructions. (i.e this is the
   format used in class and on the slides.) You are not allowed to change the
   compiler/assembler options or issue directives to permit usage of Intel's
   assembly language conventions.
*/

void _trap_entry_point(void);
void _timer_interrupt_entry_point(void);
void _keyboard_interrupt_entry_point(void);

unsigned int k_stack;
unsigned int ESP;
unsigned int timer_interrupt_flag;
unsigned int keyboard_interrupt_flag;

// Connect
void contextinit() {
    set_evec((unsigned int) 49, (unsigned long) _trap_entry_point);
    set_evec((unsigned int) TIMER_INTERRUPT_INDEX, (unsigned long) _timer_interrupt_entry_point);
    set_evec((unsigned int) KEYBOARD_INTERRUPT_INDEX, (unsigned long) _keyboard_interrupt_entry_point);
    initPIT(TIMER_FREQUENCY);
}

// Transfer control from kernel to selected process
int contextswitch(struct pcb *process) {
    deliver(process);

    // kprintf("Switching from kernel to process %d \n", process -> pid);
    ESP = process -> stack_pointer; // Stack pointer of the process to be switched into
    __asm __volatile("\
        pushf \n\
        pusha \n\
        movl %%esp, k_stack \n\
        movl ESP, %%esp \n\
        popa \n\
        iret \n\
        _keyboard_interrupt_entry_point: \n\
        cli \n\
        movl $1, keyboard_interrupt_flag \n\
        movl $0, timer_interrupt_flag \n\
        jmp _common_entry_point \n\
        _timer_interrupt_entry_point: \n\
        cli \n\
        movl $0, keyboard_interrupt_flag \n\
        movl $1, timer_interrupt_flag \n\
        jmp _common_entry_point \n\
        _trap_entry_point: \n\
        cli \n\
        movl $0, keyboard_interrupt_flag \n\
        movl $0, timer_interrupt_flag \n\
        _common_entry_point: \n\
        pusha \n\
        mov %%esp, ESP \n\
        movl k_stack, %%esp \n\
        popa \n\
        popf \n\
        "
        :
        :
        : "%eax");
    // kprintf("Switching from process %d to kernel\n", process -> pid);
    process -> stack_pointer = ESP;
    int call = ((struct context_frame *) (process -> stack_pointer)) -> eax; // extract call code from EAX register
    if (timer_interrupt_flag) {
        // Since interrupts cannot pass a call code via eax, we must overwrite the call variable
        // otherwise this just contains some garbage value of what eax was when a process was pre-empted
        call = TIMER_INTERRUPT;
    } else if (keyboard_interrupt_flag) {
        call = KEYBOARD_INTERRUPT;
    }
    
    return call; // determines which kernel service the dispatcher invokes
}

