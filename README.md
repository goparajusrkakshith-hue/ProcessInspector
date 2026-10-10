# Process CPU & Memory Inspector

## Overview

The **Process CPU & Memory Inspector** is a C-based Linux utility developed as part of the Operating Systems & Kernel Programming project. It uses the Linux `/proc` virtual filesystem to inspect process-related information and system memory details.

The project demonstrates how user-space programs can access kernel-exposed process information using standard C file-handling functions and Linux system interfaces.

## Objectives

- Understand the Linux `/proc` virtual filesystem.
- Inspect process information using a Process ID (PID).
- Read and interpret process status and CPU-related information.
- Examine system memory information.
- Apply C programming concepts, file handling, parsing, and error handling.

## Features

- **Process Inspection:** Reads available process attributes using `/proc/<PID>/status`.
- **Process Information:** Examines details such as process name, state, parent PID, and resident memory.
- **CPU Information:** Uses process statistics from `/proc/<PID>/stat` for CPU-related analysis.
- **System Memory:** Reads memory information from `/proc/meminfo`.
- **Error Handling:** Handles invalid process IDs and file-access failures.
- **Linux-Based Implementation:** Uses standard C programming and Linux process information interfaces.

## Technologies Used

- **Programming Language:** C
- **Operating System:** Linux
- **Information Source:** Linux `/proc` virtual filesystem
- **Compiler:** GCC
- **Development Environment:** Ubuntu / Linux terminal

## Project Structure

```text
ProcessInspector/
├── process_inspector.c
├── Process_CPU_Memory_Inspector.pptx
├── Process_CPU_Memory_Inspector_Review_2.pptx
├── Process_CPU_Memory_Inspector_Review_3.pptx
└── README.md
```

*The filenames listed above should match the files actually uploaded to the repository.*

## Prerequisites

Make sure GCC is installed on your Linux system.

Check the compiler version:

```bash
gcc --version
```

If GCC is not installed, use:

```bash
sudo apt update
sudo apt install build-essential
```

## Compilation

Navigate to the directory containing the C source file and compile it:

```bash
gcc -Wall -Wextra -O2 process_inspector.c -o process_inspector
```

## Execution

Run the compiled program:

```bash
./process_inspector
```

Follow the prompts displayed by the program. If it requests a Process ID, provide a valid PID for a process that you are permitted to inspect.

## Understanding the `/proc` Filesystem

| File | Purpose |
|---|---|
| `/proc/<PID>/status` | Provides human-readable process attributes. |
| `/proc/<PID>/stat` | Provides process statistics and CPU-time counters. |
| `/proc/meminfo` | Provides system-wide memory information. |

The values reported by these files can change while the system is running.

## Testing

The following checks can be performed on a Linux system:

1. Inspect an existing process.
2. Inspect the current process, if supported by the program.
3. Enter an invalid or non-existent PID and verify error handling.
4. Compare process information with the corresponding `/proc` files.
5. Compare memory information with `/proc/meminfo`.

Actual results depend on the running system, process state, permissions, and the implementation.

## Learning Outcomes

- Understanding process information exposed by the Linux kernel.
- Reading and parsing pseudo-files using C.
- Working with process identifiers and Linux process statistics.
- Applying input validation and error handling.
- Understanding the relationship between user-space applications and kernel-provided information.

## Limitations

- Process information represents a snapshot and may change during inspection.
- Access to some process details may be restricted by system permissions.
- CPU-related measurements depend on the sampling method implemented in the program.
- The utility relies on Linux-specific `/proc` interfaces.

## Future Enhancements

- Add periodic process monitoring and refreshable reports.
- Improve CPU utilization reporting and historical tracking.
- Support inspection of multiple processes.
- Add graphical visualization of CPU and memory information.
- Export process reports to text or CSV files.

## Academic Project

**Project:** Process CPU & Memory Inspector  
**Course:** Operating Systems & System Programming  
**Institution:** Koneru Lakshmaiah Education Foundation  
**Department:** Electronics and Communication Engineering

Developed as an academic project to explore Linux process management, system information, and C programming.

---

**Note:** This project is intended for educational purposes. Features described above should be retained only if they are implemented in the submitted source code.
