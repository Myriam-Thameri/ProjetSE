# Linux Multi-Process Task Scheduler
This project simulates multitasking scheduling of processes, based on a scheduling algorithms (FIFO, SJF, RR, Premptive Priority, SRT, Aging) and a set of processes under a Linux operating system.

## Project Structure 

PROJETSE
│
├── Algorithms
│ ├── Algorithms.h #Shared functions and types are definition
│ ├── RoundRobin.c
| ├── PremptivePriority.c
| ├── SJF.c
| ├── multilevel_aging.c
│
├── Config
│ ├── config.c    # Implementation of the config parser and related functions
│ ├── config.h    # Definition of CONFIG type and functions declarations
│ ├── config.txt # Declation of all processes infos
│ └── types.h     # Main data structures used in the project
│
├── Dockerfile  
├── main.c        
├── makefile
├── program
└── README.md

## Data structure 

### IO_OPERATION
**start_time:** when the I/O starts (relative to the process’s internal timeline)

**duration:** how long the I/O will block the process

### PROCESS

**ID:** identifier 

**arrival_time:** when the process arrives 

**execution_time:** total CPU time required to complete the process execution

**priority:** process priority 

**io_count:** number of active I/O operations 

**io_operations[20]:** list of all I/O operations this process will perform

### PCB

**process:** the original process data

**remaining_time:** CPU time still needed to complete the process

**executed_time:** total CPU time consumed

**io_index:** index of the next I/O operation to perform

**io_remaining:** remaining time of the current I/O operation

**wait_time** how much time the process have been waiting to get in the cpu

**finished:** boolean attribute (1 = finished, 0 = not finished)

**in_io:** boolean attribute (1 = demanding  to perform I/O, 0 = not )

### QueueNode

**process:** the process stored in this node

**next:** pointer on the next node

### QUEUE

**node:** informations of the node

**start:** pointer on the first node

**end:** pointer on the last node

**size:** number of processes in the queue