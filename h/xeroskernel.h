/* xeroskernel.h - disable, enable, halt, restore, isodd, min, max */

#ifndef XEROSKERNEL_H
#define XEROSKERNEL_H

#include <stdarg.h>

/* Symbolic constants used throughout Xinu */
typedef unsigned long PID_t;
typedef	char    Bool;        /* Boolean type                  */
typedef unsigned int size_t; /* Something that can hold the value of
                              * theoretical maximum number of bytes
                              * addressable in this architecture.
                              */
#define	FALSE   0       /* Boolean constants             */
#define	TRUE    1
#define	EMPTY   (-1)    /* an illegal gpq                */
#define	NULL    0       /* Null pointer for linked lists */
#define	NULLCH '\0'     /* The null character            */


/* Universal return constants */

#define	OK            1         /* system call ok               */
#define	SYSERR       -1         /* system call failed           */
#define	EOF          -2         /* End-of-file (usu. from read)	*/
#define	TIMEOUT      -3         /* time out  (usu. recvtim)     */
#define	INTRMSG      -4         /* keyboard "intr" key pressed	*/
                                /*  (usu. defined as ^B)        */
#define	BLOCKERR     -5         /* non-blocking op would block  */

// Test Configuration
#define BUILD_CODE 0 // 0 for Interactive Shell demo, 1 for Test Run
#define PROD_BUILD 0
#define TEST_BUILD 1

// PCB States
#define READY_0 0
#define READY_1 1
#define READY_2 2
#define READY_3 3
#define IDLE    4
#define STOPPED 5
#define RUNNING 6
#define BLOCKED 7
#define MAX_READY_PRIORITY 4
#define PRIORITY_TABLE_SIZE 6
#define PCB_TABLE_SIZE 64
#define SIGNAL_TABLE_SIZE 32
#define DEVICE_TABLE_SIZE 2
#define FILE_DESCRIPTOR_TABLE_SIZE 4

// MSG States
#define MSG_SENDING     1
#define MSG_RECEIVING   2
#define MSG_IDLE        0

// ISR Hook Indices
#define IDT_INDEX 49

// ISR Request Codes
#define CREATE      48
#define YIELD       49
#define STOP        50
#define GET_PID     51
#define PUTS        52
#define KILL        53
#define SET_PRIORITY 54
#define SEND        55
#define RECEIVE     56
#define SLEEP       57
#define TIMER_INTERRUPT 58
#define GET_CPU_TIMES 59
#define SIGNAL_RETURN 60
#define SIGNAL_HANDLER 61
#define WAIT 62
#define OPEN 63
#define CLOSE 64
#define WRITE 65
#define READ 66
#define IOCTL 67
#define KEYBOARD_INTERRUPT 68

// Process Configuration
#define SAFETY_MARGIN 16
#define DEFAULT_STACK_SIZE 1024

// PID LIMIT
#define PID_MAX 2147483647

// Timer Configuration
#define TIMER_FREQUENCY 100
#define TIMER_INTERRUPT_INDEX 32

// Device Configuration
#define DEVICE_INPUT_BUFFER_LENGTH 4
#define KEYBOARD_TYPE 0
#define KEYBOARD_NON_ECHO 0
#define KEYBOARD_ECHO 1

// Keyboard Configuration
#define KEYBOARD_IRQ 1
#define KEYBOARD_INTERRUPT_INDEX 33
#define KEYBOARD_CONTROL_PORT 0x64
#define KEYBOARD_DATA_PORT 0x60

#define ECHO_MASK 1
#define ECHO_SHIFT 0

#define EOF_FLAG_MASK 2
#define EOF_FLAG_SHIFT 1

#define EOF_MASK 0xFF000000
#define EOF_SHIFT 24
#define DEFAULT_EOF 4

#define CTRL_D 4
#define RETURN 10
#define NO_CHAR 256

/* Functions defined by startup code */


void           bzero(void *base, int cnt);
void           bcopy(const void *src, void *dest, unsigned int n);
void           disable(void);
unsigned short getCS(void);
unsigned char  inb(unsigned int);
void           init8259(void);
int            kprintf(char * fmt, ...);
void           lidt(void);
void           outb(unsigned int, unsigned char);
void           set_evec(unsigned int xnum, unsigned long handler);

struct memHeader {
    unsigned long size;
    struct memHeader *next;
    struct memHeader *prev;
    char *sanityCheck;
    unsigned char dataStart[0];
};

struct pcb {
    unsigned int index;
    unsigned long pid;
    unsigned long parent_id;
    unsigned int state;
    unsigned int priority;
    unsigned long stack_pointer;
    unsigned long process_memory;
    unsigned long process_memory_end;
    struct pcb *next;

    unsigned long ticks_delayed;

    struct pcb* sender_queue;
    struct pcb* sender_next;
    unsigned int messenger_state;
    unsigned long message;
    unsigned long messenger;

    unsigned long cpu_ticks;

    unsigned long signal_handlers[SIGNAL_TABLE_SIZE];
    unsigned long running_signals;
    unsigned long pending_signals;

    struct pcb *wait_target;

    struct file_descriptor *file_descriptors[FILE_DESCRIPTOR_TABLE_SIZE];

    char *input_buffer;
    unsigned long input_buffer_current_length;
    unsigned long input_buffer_max_length;
};

struct context_frame {
    unsigned long edi;
    unsigned long esi;
    unsigned long ebp;
    unsigned long esp;
    unsigned long ebx;
    unsigned long edx;
    unsigned long ecx;
    unsigned long eax;
    unsigned long iret_eip;
    unsigned long iret_cs;
    unsigned long eflags;
    unsigned long return_address;
};

struct trampoline_arguments {
    unsigned long handler;
    unsigned long context;
};

struct process_status_list {
  int length;                   // Last entry used in the table
  int pid[PCB_TABLE_SIZE];      // The process ID
  int status[PCB_TABLE_SIZE];   // The process status
  long cpuTime[PCB_TABLE_SIZE]; // CPU time used in milliseconds
};

struct device {
    int device_major_number;
    int device_minor_number;

    char *device_name;
    unsigned long device_init; // int keyboard_init(struct device *keyboard, int echoing) 
    unsigned long device_open;
    unsigned long device_close;
    unsigned long device_read;
    unsigned long device_write;
    unsigned long device_cntl;
    unsigned long state;

    unsigned long owner_pid;

    unsigned long input_buffer[DEVICE_INPUT_BUFFER_LENGTH];
    unsigned long input_buffer_current_length;
    unsigned long input_buffer_max_length;

    // int (*device_seek)(void);
    // int (*device_getc)(void);
    // int (*device_putc)(void);
    // int (*device_iint)(void);
    // int (*device_oint)(void);

    // void *device_csr;
    // void *device_ivec;
    // void *device_ovec;
    // void *device_io_block;
};

struct file_descriptor {
    int device_major_number;
    int device_minor_number;
    struct device *device;
};

// Memory Manager API
int is_in_memory_space(unsigned long address);
void kmeminit(void);
void *kmalloc(size_t size);
int kfree(void *ptr);
unsigned long kmem_level(void);
void kmem_print(void);

// Dispatcher / Process Manager API
void dispatchinit(void);
void dispatch(void);
void pause(unsigned int period);

// Context Switcher API
void contextinit(void);
int contextswitch(struct pcb *process);

// Process Creation API
int create(void (*func)(void), int stack);
struct pcb *pcb_lookup(unsigned long pid);
struct pcb *next(void);
struct pcb *stopped(void);
struct pcb *remove(struct pcb *targetPCB);
void block(struct pcb *targetPCB);
void ready(struct pcb *process);
void stop(struct pcb *process);
struct pcb *create_process(void (*func)(void), unsigned long stack_size, unsigned long parent_id);
int destroy_process(struct pcb *process);
void process_print(void);
void processinit(void);
void set_return_value(struct pcb *process, unsigned long return_value);
va_list *get_system_arguments(struct pcb *process);
int process_owns_memory(struct pcb *process, unsigned long address);
int pcb_alone(struct pcb *process);
int process_statuses(struct process_status_list *status_list);
void print_process_statuses(struct process_status_list *status_list);
void unblock(struct pcb *process);

// Interprocess Messaging API
int send(struct pcb *sender, unsigned long dest_pid, unsigned long num);
int recv(struct pcb *receiver, unsigned long *src_pid, unsigned long *num);
void cleanup_messages(struct pcb* process);
void unsend(struct pcb *process);
void unreceive(struct pcb *process);

// Sleep Device API
void sleep(struct pcb *process, unsigned int milliseconds);
void tick(struct pcb *process);
unsigned long get_ticks(void);
unsigned int ticks_remaining(struct pcb *process);
unsigned int wake(struct pcb * process);

// System Service API
int syscall(int call, int size, ...);
unsigned int syscreate(void (*func)(void), int stack);
void sysyield(void);
void sysstop(void);
int syssend(unsigned int dest_pid, unsigned long num);
int sysrecv(unsigned int *src_pid, unsigned int *num);
PID_t sysgetpid(void);
void sysputs(char *str);
int syskill(PID_t pid, int signal_number);
int syssetprio(int priority);
unsigned int syssleep(unsigned int milliseconds);
int sysgetcputimes(struct process_status_list* status_list);
void syssigreturn(void *context);
int syssighandler(int signal, void (*new_handler)(void *), void (** old_handler)(void *));
int syswait(PID_t pid);
int sysopen(int device_no);
int sysclose(int fd);
int syswrite(int fd, void *buff, int bufflen);
int sysread(int fd, void *buff, int bufflen);
int sysioctl(int fd, unsigned long command, ...);

// User Processes
void root(void);
void producer(void);
void consumer(void);
void idleproc(void);
void test_process(void);

// Signals
void unwait(struct pcb *process);
int wait(struct pcb *process, struct pcb *target);
int sighandler(int signal, struct pcb * process, void (*new_handler)(void *), void (** old_handler)(void *));
void sigreturn(struct pcb *process, struct context_frame *context);
void sigtramp(void (*handler)(void *), void *context);
void deliver(struct pcb *process);
int signal(int signal_number, struct pcb *target_process);
void terminate(void *context);

// Device Table Manager
void deviceinit(void);
struct device *find_device(int index);

// Device Independent Calls
void di_bind(struct pcb *process, struct device *device, unsigned long file_descriptor);
void di_unbind(struct pcb *process, unsigned long file_descriptor);
int di_open(struct pcb *process, int device_number);
int di_close(struct pcb *process, int file_descriptor);
int di_write(struct pcb *process, int file_descriptor, void *buffer, int buffer_length);
int di_read(struct pcb *process, int file_descriptor, void *buffer, int buffer_length);
int di_ioctl(struct pcb *process, int file_descriptor, unsigned long command, int size, va_list *device_argument_list);

// Keyboard Functions
struct device *get_active_keyboard(void);
void set_active_keyboard(struct device *keyboard);
int device_is_keyboard(struct device *keyboard);

int get_keyboard_echoing(struct device *keyboard);
void set_keyboard_echoing(struct device *keyboard, unsigned long value);
unsigned long get_keyboard_eof(struct device *keyboard);
void set_keyboard_eof(struct device *keyboard, unsigned long value);
unsigned long get_keyboard_eof_flag(struct device *keyboard);
void set_keyboard_eof_flag(struct device *keyboard, unsigned long value);

void read_character(struct pcb *process, struct device *keyboard, unsigned long ascii_code);
int keyboard_init(struct device *keyboard);
int keyboard_open(struct device *keyboard);
int keyboard_close(struct device *keyboard);
int keyboard_read(struct pcb *process, struct device *keyboard, void *buffer, int buffer_length);
int keyboard_write(struct pcb *process, struct device *keyboard, void *buffer, int buffer_length);
int keyboard_control(struct device *keyboard, unsigned long command, int size, va_list *device_arguments_list);
void keyboard_tick(struct device *keyboard);
void unread(struct pcb *process);

// String Functions
unsigned int kstrcmp(char *a, char *b);
void kstrcopy(char *a, char *b, unsigned long len);
char *kstrtok(char *new_token_string, char delimiter);
unsigned long kstrinlen(char *s);
unsigned long kstrtoi(char *s);

// Shell Functions
void shell(void);
void ps(void);
void ex(void);
void k(void);
void a(void);
void t(void);

/* Anything you add must be between the #define and this comment */
#endif
