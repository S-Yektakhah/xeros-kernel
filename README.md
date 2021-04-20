# xeros-kernel

Code for UBC CPSC 415 Advanced Operating Systems capstone project. The project is an x86 kernel that can run on the Bochs IA-32 emulator. Only code written by my partner
and I are provided.

| **Kernel Component**        | **File**                    | **Description**                                                                                       |
|-----------------------------|-----------------------------|-------------------------------------------------------------------------------------------------------|
| Kernel Initialization       | init.c                      | Initialize data structures belonging to kernel (ex: Process Table, Interrupt Table, Memory List, etc. |
| Memory Manager              | mem.c                       | Allocate and deallocate memory from system                                                            |
| Dispatcher                  | disp.c                      | Schedules the next ready process, calls ctsw.c to switch                                              |
| Context Switcher            | ctsw.c                      | Context switch between the kernel and process                                                         |
| Process Creation            | create.c                    | Creates a process and places it on a ready queue                                                      |
| Root Process                | user.c , shell.c            | Simple shell process and small testing suite                                                          |
| System Calls                | syscall.c                   | System calls (ex: yield, stop, kill process, etc.)                                                    |
| Signals                     | signal.c                    | Allows kernel to asynchronously signal a process                                                      |
| Inter-Process Communication | msg.c, sleep.c              | Allows processes to send and receive information from one another                                     |
| Device Drivers              | device.c, di_calls.c, kbd.c | Device independent interface for attaching any device and a working keyboard device                   |
| Header Files                | xeroskernel.h, kbd.h        |                                                                                                       |
