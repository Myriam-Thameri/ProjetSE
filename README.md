# Linux Multi-Process Task Scheduler
This project simulates multitasking scheduling of processes, based on a scheduling algorithms (FIFO, SJF, RR, Premptive Priority, SRT, Aging) and a set of processes under a Linux operating system.

## Project Structure 

PROJETSE<br>
│<br>
├── Algorithms<br>
│ ├── First_In_First_Out<br>
│ ├── Round_Robin.c<br>
| ├── Premptive_Priority.c<br>
| ├── Shortest_Job_First.c<br>
| ├── Shortest_Remaining_Time.c<br>
| ├── Multilevel_Static.c<br>
| ├── Multilevel_Aging.c<br>
│<br>
├── Config<br>
│ ├── config.c                 # Implementation of the config parser and related functions<br>
│ ├── config.h                 # Definition of CONFIG type and functions declarations<br>
│ ├── config.txt               # Declation of all processes infos<br>
│ └── types.h                  # Main data structures used in the project<br>
│<br>
├── Interface<br>
│ ├── gantt_chart.h            # Definition of the gant diagram function<br>
│ ├── gantt_chart.c            # Impelmentation of functions allowing gant diagram drawing<br>
│ ├── interface_utils.h        # Definition of AppContext type and interface related functions declarations<br>
│ └── interface.c              # Main functionnalities used to build the interface<br>
│<br>
├── output                     # Folder to store the log files<br>
│ └── {algorithm used}_{configfile used}.log      # Pattern of the log filename<br>
│<br>
├── Utils<br>
│ ├── Algorithms.h            # Definition of shared functions and types used in algorithms implementation<br>
│ ├── Algorithms.c            # Impelmentation of shared functions used by algorithms files<br>
│ ├── utils.h                 # Declarations of additional functions used to build the scheduler app<br>
│ ├── utils.c                 # Impelmentation of functions defined in utils.h<br>
│ ├── log_file.h              # Definition of function used to build the logfile logic<br>
│ └── log_file.c              # Implementation of the logfile logic<br>
│<br>
├── Dockerfile  <br>
├── main.c        <br>
├── makefile<br>
├── program<br>
└── README.md<br>

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

## Input/Output Operations Policy : FIFO

## Priority Management 

The process having the highest priority is the lost prioritized one.

##  License

This project is licensed under the MIT License.  
See the [LICENSE](LICENSE) file for more details.

### About MIT License
The MIT License allows:
- Commercial use
- Distribution
- Modification
- Private use

The only requirement is to include the copyright notice and the license text.

## Authors

- Safa Drissi
- Myriam Thameri
- Arij Belmabrouk
- Amira Rabah
- Ikram Marzouki
- Yasmine Hsayri
- Wassim Lourimi

Supervised by: Mme Yosra Najar  
Academic Year: 2025–2026  
Institution: Université Virtuelle de Tunis
