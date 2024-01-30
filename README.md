# OS-Course-Exercises
Repository for exercises and assignments completed during the Operating Systems course at TAU

## Exercise 1: Multi-level Page Table Simulation

### Introduction
The goal of this assignment is to implement simulated OS code that handles a multi-level (trie-based) page table. Two functions need to be implemented: one to create/destroy virtual memory mappings in a page table, and another to check if an address is mapped in a page table. The code runs in a normal process and simulates OS functionality. The provided `os.c` and `os.h` files contain helper functions for OS simulation.

### Target Hardware
The simulated OS targets an imaginary 64-bit x86-like CPU. Virtual addresses have a size of 64 bits, with only the lower 57 bits used for translation. Physical addresses are also 64 bits. Page tables use a 4KB page/frame size, and the page table entry size is 64 bits.

### Implemented Functions
1. **page_table_update**: Creates/destroys virtual memory mappings in a page table.
    ```c
    void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn);
    ```
2. **page_table_query**: Queries the mapping of a virtual page number in a page table.
    ```c
    uint64_t page_table_query(uint64_t pt, uint64_t vpn);
    ```
### Running
The program compiles with the following command:
```bash
gcc -O3 -Wall -std=c11 os.c pt.c
```


## Exercise 2: Simple Shell Implementation

### Introduction
This assignment involves implementing a simple shell program to gain experience with process management, pipes, signals, and relevant system calls. The provided `shell.c` skeleton reads user commands, parses them, and invokes specified functions.

### Shell Skeleton
The provided skeleton executes an infinite loop, reading shell commands from standard input, parsing them into commands, and invoking your function. It supports executing commands, executing commands in the background, single piping, and output redirection.

### Shell Functionality
- **Executing Commands:** Enter a command, i.e., a program and its arguments (e.g., `sleep 10`). The shell executes the command and waits until it completes before accepting another command.
- **Background Execution:** Append `&` to a command (e.g., `sleep 10 &`) for background execution. The shell does not wait for completion.
- **Single Piping:** Use the pipe symbol (`|`) to execute two commands concurrently, piping the output of the first to the input of the second (e.g., `cat foo.txt | grep bar`).
- **Output Redirecting:** Redirect standard output to a file using `>` (e.g., `cat foo > file.txt`). The shell waits for command completion.

### Assignment Functions
Implement the following functions in a file named `myshell.c`: `prepare()`, `process_arglist()`, and `finalize()`.

#### `prepare()`
This function is called before the first invocation of `process_arglist()`. Return 0 on success; any other value indicates an error. Use it for necessary initialization.

#### `process_arglist(int count, char **arglist)`
This function receives an array `arglist` with `count` non-NULL words. Execute commands as child processes using `fork()` and `execvp()`. Support executing commands in the background, piping, and output redirection.

#### `finalize()`
This function is called before exiting. Return 0 on success; any other value indicates an error. Use it for cleanup related to `process_arglist()`.

### Running the Shell
Compile the program using:
```bash
gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 shell.c myshell.c -o myshell
```
Run the shell:
```bash
./myshell
```


## Exercise 3: Kernel Module and IPC Mechanism

### Introduction
This assignment involves implementing a kernel module providing a new IPC mechanism called a message slot. A message slot is a character device file for inter-process communication.

### Message Slot Specification
- A message slot appears as a character device file with major number 240.
- Multiple message slots can exist, each distinguished by a unique minor number.
- The module supports ioctl, write, and read operations for channel-based communication.

### Assignment Components
1. **message_slot.c & message_slot.h:** Implement the kernel module.
   - Use major number 240.
   - Implement ioctl, write, and read operations with specified semantics.
   - Handle memory allocation and deallocation properly.
2. **message_sender.c:** User space program to send messages.
    Command line arguments: path to message slot file, target channel id, and message.
    Open the specified file, set channel id, write the message, close the device
3. **message_reader.c:** User space program to read messages.
    Command line arguments: path to message slot file and target channel id.
    Open the specified file, set channel id, read the message, print it, and close the device.

### Compilation and Running (Kernel Module)
```bash
# Compile kernel module
make -C /lib/modules/$(uname -r)/build M=$PWD modules

# Load the kernel module
sudo insmod message_slot.ko

# Compile message_sender
gcc -O3 -Wall -std=c11 message_sender.c -o message_sender

# Run message_sender
./message_sender /dev/slot0 1 "Hello, Channel 1"

# Compile message_reader
gcc -O3 -Wall -std=c11 message_reader.c -o message_reader

# Run message_reader
./message_reader /dev/slot0 1

```
### Example Session
1. Load the kernel module.
2. Create a message slot file using mknod: mknod /dev/slot0 c 240 0.
3. Change permissions to make it readable/writable: chmod 666 /dev/slot0.
4. Use message_sender and message_reader to send/read messages on various channels.



## Exercise 4: Toy Client/Server Architecture - Printable Characters Counting (PCC)

### Introduction
This assignment involves implementing a client/server architecture for a Printable Characters Counting (PCC) server. Clients send a stream of bytes to the server, which counts the printable characters and returns the count. The server also maintains overall statistics, printing them on termination.

### Client (pcc_client.c)
### Compilation and Running
```bash
# Compile pcc_client
gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 pcc_client.c -o pcc_client

# Run pcc_client
./pcc_client <server_ip> <server_port> <file_path>
```
### Server (pcc_server.c)
### Compilation and Running
```bash
# Compile pcc_server
gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 pcc_server.c -o pcc_server

# Run pcc_server
./pcc_server <server_port>
```


