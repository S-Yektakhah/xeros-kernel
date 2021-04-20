#include <xeroskernel.h>
#include <xeroslib.h>

struct device devices[DEVICE_TABLE_SIZE];

extern void deviceinit() {
    int index;
     struct device *device_entry;

    index = 0;
    device_entry = &devices[index];
    device_entry -> device_major_number = KEYBOARD_TYPE;
    device_entry -> device_minor_number = KEYBOARD_NON_ECHO;
    device_entry -> input_buffer_current_length = NULL;
    device_entry -> input_buffer_max_length = DEVICE_INPUT_BUFFER_LENGTH;
    keyboard_init(device_entry);

    index = 1;
    device_entry = &devices[index];
    device_entry -> device_major_number = KEYBOARD_TYPE;
    device_entry -> device_minor_number = KEYBOARD_ECHO;
    device_entry -> input_buffer_current_length = NULL;
    device_entry -> input_buffer_max_length = DEVICE_INPUT_BUFFER_LENGTH;
    keyboard_init(device_entry);

    return;
}

extern struct device *find_device(int index) {
    return &devices[index];
}