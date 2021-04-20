#include <xeroskernel.h>
#include <stdarg.h>

extern void di_bind(struct pcb *process, struct device *device, unsigned long file_descriptor) {
    struct file_descriptor *descriptor = kmalloc(sizeof(struct file_descriptor));
    process -> file_descriptors[file_descriptor] = descriptor;

    descriptor -> device_major_number = device -> device_major_number;
    descriptor -> device_minor_number = device -> device_minor_number;
    descriptor -> device = device;

    device -> owner_pid = process -> pid;

    void (*device_open)(struct device *) = (void (*)(struct device *))  device -> device_open;
    device_open(device);
    return;
}

extern void di_unbind(struct pcb *process, unsigned long file_descriptor) {
    struct file_descriptor *descriptor = process -> file_descriptors[file_descriptor];
    struct device *device = (struct device *) descriptor -> device;

    kfree(descriptor);
    process -> file_descriptors[file_descriptor] = NULL;
    device -> owner_pid = NULL;

    void (*device_close)(struct device *)  = (void (*)(struct device *)) device -> device_close;
    device_close(device);
    return;
}

extern int di_open(struct pcb *process, int device_number) {
    if (device_number < 0 || DEVICE_TABLE_SIZE <= device_number) {
        return -1; // Invalid Device Number
    }

    struct device *device = find_device(device_number);
    if (device -> owner_pid != NULL) {
        return -1; // Device already in use
    }

    int file_descriptor;
    for (file_descriptor = 0; file_descriptor < FILE_DESCRIPTOR_TABLE_SIZE; file_descriptor++) {
        if (process -> file_descriptors[file_descriptor] == NULL) { break; }
    }

    if (file_descriptor >= FILE_DESCRIPTOR_TABLE_SIZE) {
        return -1; // No File Descriptor Entry Available
    } else {
        
        if (device_is_keyboard(device)) {
            if (get_active_keyboard() != NULL) {
                return -1; // Keyboard already open, cannot open multiple keyboards
            } else {
                set_active_keyboard(device);
            }
        }

        di_bind(process, device, file_descriptor);
        return file_descriptor;
    }
}
extern int di_close(struct pcb *process, int file_descriptor) {

    if (file_descriptor < 0 || FILE_DESCRIPTOR_TABLE_SIZE <= file_descriptor || process -> file_descriptors[file_descriptor] == NULL) {
        return -1; // Invalid Device Number
    } else {
        struct file_descriptor *descriptor = process -> file_descriptors[file_descriptor];
        struct device *device = descriptor -> device;

        if (device -> owner_pid == NULL) {
            return -1; // Device already closed
        }

        if (device_is_keyboard(device)) {
            if (get_active_keyboard() == NULL) {
                kprintf("ERROR: Keyboard is open but keyboard flag is set to false\n");
                return -1;
            } else {
                set_active_keyboard(NULL);
            }
        }

        di_unbind(process, file_descriptor);
        return 0;
    }
}
extern int di_write(struct pcb *process, int file_descriptor, void *buffer, int buffer_length) {
    if (file_descriptor < 0 || FILE_DESCRIPTOR_TABLE_SIZE <= file_descriptor || process -> file_descriptors[file_descriptor] == NULL) {
        return -1; // Invalid file_descriptor
    }
    if (process_owns_memory(process, (unsigned long) buffer) == 0) {
        return -1; // Invalid buffer address
    }
    if (buffer_length < 0) {
        return -1;
    }
    struct file_descriptor *descriptor = process -> file_descriptors[file_descriptor];
    struct device *device = descriptor -> device;
    int (*device_write)(struct pcb *, struct device *, void *, int) = (int (*)(struct pcb *, struct device *, void *, int)) device -> device_write;

    return device_write(process, device, buffer, buffer_length);
}
extern int di_read(struct pcb *process, int file_descriptor, void *buffer, int buffer_length) {
    if (file_descriptor < 0 || FILE_DESCRIPTOR_TABLE_SIZE <= file_descriptor || process -> file_descriptors[file_descriptor] == NULL) {
        return -1; // Invalid file_descriptor
    }
    if (process_owns_memory(process, (unsigned long) buffer) == 0) {
        return -1; // Invalid buffer address
    }
    if (buffer_length < 0) {
        return -1;
    }
    struct file_descriptor *descriptor = process -> file_descriptors[file_descriptor];
    struct device *device = descriptor -> device;
    int (*device_read)(struct pcb *, struct device *, void *, int) = (int (*)(struct pcb *, struct device *, void *, int)) device -> device_read;

    return device_read(process, device, buffer, buffer_length);
}
extern int di_ioctl(struct pcb *process, int file_descriptor, unsigned long command, int size, va_list *device_argument_list) {
    if (file_descriptor < 0 || FILE_DESCRIPTOR_TABLE_SIZE <= file_descriptor || process -> file_descriptors[file_descriptor] == NULL) {
        return -1; // Invalid file_descriptor
    }

    struct file_descriptor *descriptor = process -> file_descriptors[file_descriptor];
    struct device *device = descriptor -> device;
    int (*device_control)(struct device *, unsigned long, int, va_list *) = (int (*)(struct device *, unsigned long, int, va_list *) ) device -> device_cntl;

    return device_control(device, command, size, device_argument_list);
}