#include <xeroskernel.h>
#include <kbd.h>
#include <stdarg.h>
#include <i386.h>


#define KEY_UP   0x80            /* If this bit is on then it is a key   */
                                 /* up event instead of a key down event */

/* Control code */
#define LSHIFT  0x2a
#define RSHIFT  0x36
#define LMETA   0x38

#define LCTL    0x1d
#define CAPSL   0x3a

/* scan state flags */
#define INCTL           0x01    /* control key is down          */
#define INSHIFT         0x02    /* shift key is down            */
#define CAPSLOCK        0x04    /* caps lock mode               */
#define INMETA          0x08    /* meta (alt) key is down       */
#define EXTENDED        0x10    /* in extended character mode   */

#define EXTESC          0xe0    /* extended character escape    */
#define NOCHAR  256

static  int     state; /* the state of the keyboard */

/*  Normal table to translate scan code  */
unsigned char   kbcode[] = { 0,
          27,  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',
         '0',  '-',  '=', '\b', '\t',  'q',  'w',  'e',  'r',  't',
         'y',  'u',  'i',  'o',  'p',  '[',  ']', '\n',    0,  'a',
         's',  'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';', '\'',
         '`',    0, '\\',  'z',  'x',  'c',  'v',  'b',  'n',  'm',
         ',',  '.',  '/',    0,    0,    0,  ' ' };

/* captialized ascii code table to tranlate scan code */
unsigned char   kbshift[] = { 0,
           0,  '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',
         ')',  '_',  '+', '\b', '\t',  'Q',  'W',  'E',  'R',  'T',
         'Y',  'U',  'I',  'O',  'P',  '{',  '}', '\n',    0,  'A',
         'S',  'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  '"',
         '~',    0,  '|',  'Z',  'X',  'C',  'V',  'B',  'N',  'M',
         '<',  '>',  '?',    0,    0,    0,  ' ' };
/* extended ascii code table to translate scan code */
unsigned char   kbctl[] = { 0,
           0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
           0,   31,    0, '\b', '\t',   17,   23,    5,   18,   20,
          25,   21,    9,   15,   16,   27,   29, '\n',    0,    1,
          19,    4,    6,    7,    8,   10,   11,   12,    0,    0,
           0,    0,   28,   26,   24,    3,   22,    2,   14,   13 };

// static int
// extchar(code)
// unsigned char   code;
// {
//         state &= ~EXTENDED;
// }

unsigned int kbtoa( unsigned char code ) {
  unsigned int    ch;
  
  if (state & EXTENDED)
    // return extchar(code);
    return NOCHAR;
  if (code & KEY_UP) {
    switch (code & 0x7f) {
    case LSHIFT:
    case RSHIFT:
      state &= ~INSHIFT;
      break;
    case CAPSL:
    //   printf("Capslock off detected\n");
      state &= ~CAPSLOCK;
      break;
    case LCTL:
      state &= ~INCTL;
      break;
    case LMETA:
      state &= ~INMETA;
      break;
    }
    
    return NOCHAR;
  }
  
  /* check for special keys */
  switch (code) {
  case LSHIFT:
  case RSHIFT:
    state |= INSHIFT;
    // printf("shift detected!\n");
    return NOCHAR;
  case CAPSL:
    state |= CAPSLOCK;
    // printf("Capslock ON detected!\n");
    return NOCHAR;
  case LCTL:
    state |= INCTL;
    return NOCHAR;
  case LMETA:
    state |= INMETA;
    return NOCHAR;
  case EXTESC:
    state |= EXTENDED;
    return NOCHAR;
  }
  
  ch = NOCHAR;
  
  if (code < sizeof(kbcode)){
    if ( state & CAPSLOCK )
      ch = kbshift[code];
	  else
	    ch = kbcode[code];
  }
  if (state & INSHIFT) {
    if (code >= sizeof(kbshift))
      return NOCHAR;
    if ( state & CAPSLOCK )
      ch = kbcode[code];
    else
      ch = kbshift[code];
  }
  if (state & INCTL) {
    if (code >= sizeof(kbctl))
      return NOCHAR;
    ch = kbctl[code];
  }
  if (state & INMETA)
    ch += 0x80;
  return ch;
}

int keyboard_state = 0;
struct device *active_keyboard = NULL;

void push_buffer(unsigned long buffer[], unsigned long *current_length, unsigned long max_length, unsigned long value);
void push_byte_buffer(char buffer[], unsigned long *current_length, unsigned long max_length, unsigned long value);
unsigned long pop_buffer(unsigned long buffer[], unsigned long *current_length);
void read_character(struct pcb *process, struct device *keyboard, unsigned long ascii_code);

void print_byte_buffer(char buffer[], unsigned long length, char *name) {
    kprintf("%s Content: '", name);
    int i;
    for (i = 0; i < length; i++) {
        kprintf("%c", buffer[i]);
    }
    kprintf("'\n");
}

void print_buffer(unsigned long buffer[], unsigned long length, char *name) {
    kprintf("%s Content: '", name);
    int i;
    for (i = 0; i < length; i++) {
        kprintf("%c", buffer[i]);
    }
    kprintf("'\n");
}

void push_buffer(unsigned long buffer[], unsigned long *current_length, unsigned long max_length, unsigned long value) {
    if (*current_length >= max_length) { return; }
    buffer[*current_length] = value;
    *current_length = *current_length + 1;
    return;
}

void push_byte_buffer(char buffer[], unsigned long *current_length, unsigned long max_length, unsigned long value) {
    if (*current_length >= max_length) { return; }
    buffer[*current_length] = value;
    *current_length = *current_length + 1;
    return;
}

unsigned long pop_buffer(unsigned long buffer[], unsigned long *current_length) {
    if (*current_length <= 0) { return -1; }
    unsigned long value = buffer[0];
    int i;
    for (i = 0; i < (*current_length)-1; i++) {
        buffer[i] = buffer[i+1];
    }
    *current_length = *current_length - 1;
    return value;
}

extern void read_character(struct pcb *process, struct device *keyboard, unsigned long ascii_code) {
    // kprintf("Test Char: %c\n", ascii_code);

    if (get_keyboard_echoing(keyboard) == 1) { kprintf("%c", ascii_code); }

    if (keyboard -> input_buffer_current_length == keyboard -> input_buffer_max_length) { return; }

    // Assume: ascii_code is not 256 and Kernel Buffer is not full
    if (ascii_code == get_keyboard_eof(keyboard)) {
        // kprintf("Keyboard EOF detected. Disabling keyboard interrupts.\n");
        set_keyboard_eof_flag(keyboard, 1); // Let's sysread know that EOF occured
        enable_irq(KEYBOARD_IRQ, 1); // Disabled keyboard interrupts
    }

    if (process -> input_buffer != NULL) { // Transfer into Process Buffer
        // kprintf("Transfering to Process Buffer\n");
        if (ascii_code != get_keyboard_eof(keyboard)) {
            push_byte_buffer(process -> input_buffer, &(process -> input_buffer_current_length), process -> input_buffer_max_length, ascii_code);
            // print_byte_buffer(process -> input_buffer, process -> input_buffer_current_length, "Process Buffer");
        }
        // Handle unblocking
        if (get_keyboard_eof_flag(keyboard) == 1 || ascii_code == RETURN || process -> input_buffer_current_length == process -> input_buffer_max_length) {
            set_return_value(process, process -> input_buffer_current_length);
            process -> input_buffer = NULL;
            process -> input_buffer_current_length = NULL;
            process -> input_buffer_max_length = NULL;
            ready(process);
        }
    } else { // Transfer into Kernel Buffer
        // kprintf("Transfering to Kernel Buffer\n");
        push_buffer(keyboard -> input_buffer, &(keyboard -> input_buffer_current_length), keyboard -> input_buffer_max_length, ascii_code);
        // print_buffer(keyboard -> input_buffer, keyboard -> input_buffer_current_length, "Kernel Buffer");
    }
}

extern struct device *get_active_keyboard() {
    return active_keyboard;
}

extern void set_active_keyboard(struct device *keyboard) {
    active_keyboard = keyboard;
    return;
}

extern int device_is_keyboard(struct device *keyboard) {
    return keyboard -> device_major_number == KEYBOARD_TYPE;
}

extern int get_keyboard_echoing(struct device *keyboard) {
    return (((keyboard -> state) & ECHO_MASK) >> ECHO_SHIFT);
}

extern void set_keyboard_echoing(struct device *keyboard, unsigned long value) {
    keyboard -> state &= ~ECHO_MASK;
    keyboard -> state |= value << ECHO_SHIFT;
    return;
}

extern unsigned long get_keyboard_eof(struct device *keyboard) {
    return (((keyboard -> state) & EOF_MASK) >> EOF_SHIFT);
}

extern void set_keyboard_eof(struct device *keyboard, unsigned long value) {
    keyboard -> state &= ~EOF_MASK;
    keyboard -> state |= value << EOF_SHIFT;
    return;
}

extern unsigned long get_keyboard_eof_flag(struct device *keyboard) {
    return (((keyboard -> state) & EOF_FLAG_MASK) >> EOF_FLAG_SHIFT);
}

extern void set_keyboard_eof_flag(struct device *keyboard, unsigned long value) {
    keyboard -> state &= ~EOF_FLAG_MASK;
    keyboard -> state |= value << EOF_FLAG_SHIFT;
    return;
}

extern int keyboard_init(struct device *keyboard) {
    keyboard -> owner_pid = NULL;
    keyboard -> device_name = NULL;
    keyboard -> state = NULL;
    keyboard -> device_init = (unsigned long) keyboard_init;
    keyboard -> device_open = (unsigned long) keyboard_open;
    keyboard -> device_close = (unsigned long) keyboard_close;
    keyboard -> device_read = (unsigned long) keyboard_read;
    keyboard -> device_write = (unsigned long) keyboard_write;
    keyboard -> device_cntl = (unsigned long) keyboard_control;
    return 0;
}

extern int keyboard_open(struct device *keyboard) {
    keyboard -> input_buffer_current_length = 0;
    set_keyboard_eof(keyboard, DEFAULT_EOF);
    set_keyboard_eof_flag(keyboard, 0);

    if (keyboard -> device_minor_number == KEYBOARD_NON_ECHO) {
        keyboard -> device_name = "keyboard_non_echo";
        set_keyboard_echoing(keyboard, KEYBOARD_NON_ECHO);
    } else if (keyboard -> device_minor_number == KEYBOARD_ECHO) {
        keyboard -> device_name = "keyboard_echo";
        set_keyboard_echoing(keyboard, KEYBOARD_ECHO);
    }

    enable_irq(KEYBOARD_IRQ, 0);
    return 0;
};

extern int keyboard_close(struct device *keyboard) {
    keyboard -> input_buffer_current_length = 0;
    enable_irq(KEYBOARD_IRQ, 1);
    return 0;
};

extern void unread(struct pcb *process) {
    process -> input_buffer = NULL;
    ready(process);
    return;
}

extern int keyboard_read(struct pcb *process, struct device *keyboard, void *buffer, int buffer_length) {

    unsigned long input_buffer_current_length = 0;
    unsigned long value;
    while (keyboard -> input_buffer_current_length > 0 && input_buffer_current_length < buffer_length) {
        value = pop_buffer(keyboard -> input_buffer, &(keyboard -> input_buffer_current_length));

        if (value != get_keyboard_eof(keyboard)) {
            push_byte_buffer(buffer, &input_buffer_current_length, buffer_length, value);
        }

        if (value == get_keyboard_eof(keyboard) || value == RETURN) {
            break;
        }
    }

    if (input_buffer_current_length >= buffer_length || value == get_keyboard_eof(keyboard) || value == RETURN) {
        // Return syscall immediately
        return input_buffer_current_length;
    } else {
        // Set up PCB for a blocking sysread
        process -> input_buffer = buffer;
        process -> input_buffer_current_length = input_buffer_current_length;
        process -> input_buffer_max_length = buffer_length;
        block(process);
        return -420;
    }
};

extern int keyboard_write(struct pcb *process, struct device *keyboard, void *buffer, int buffer_length) {
    return -1;
};

extern int keyboard_control(struct device *keyboard, unsigned long command, int size, va_list *device_arguments_list) {
    int return_code = 0;
    switch(command) {
        case 53:;
            char new_eof = (char) (va_arg(*device_arguments_list, int));
            set_keyboard_eof(keyboard, (unsigned long) new_eof);
            // kprintf("New EOF = %c\n", get_keyboard_eof(keyboard));
            break;
        case 55:
            set_keyboard_echoing(keyboard, 0);
            // kprintf("New Echoing = %d\n", get_keyboard_echoing(keyboard));            
            break;
        case 56:
            set_keyboard_echoing(keyboard, 1);
            // kprintf("New Echoing = %d\n", get_keyboard_echoing(keyboard));            
            break;
        default:
            // kprintf("ERROR: Unsupported Keyboard Control Code %d\n", command);
            return_code = -1;
            break;
    }
    return return_code;
};

extern void keyboard_tick(struct device *keyboard) {
    // if (keyboard == NULL) { kprintf("No Active Keyboard\n"); }
    // else if (keyboard -> owner_pid == NULL) { kprintf("Keyboard not owned\n"); }
    // else if ((inb(KEYBOARD_CONTROL_PORT) & 1) == 0) { kprintf("No character to read\n"); }
    // else { kprintf("Keyboard: %d -> %c\n", inb(KEYBOARD_DATA_PORT), kbtoa(inb(KEYBOARD_DATA_PORT))); }

    if (keyboard == NULL || keyboard -> owner_pid == NULL) {
        // kprintf("No Active Keyboard or Keyboard not owned\n");
        return;
    }

    unsigned long control_code = inb(KEYBOARD_CONTROL_PORT);
    if ((control_code & 1) != 1) {
        // kprintf("No character to read\n");
        return;
    }

    struct pcb *process = pcb_lookup(keyboard -> owner_pid);
    int scan_code = inb(KEYBOARD_DATA_PORT);
    unsigned long ascii_code = kbtoa(scan_code);

    if (ascii_code == 256) { return; }
    read_character(process, keyboard, ascii_code);
    return;
}