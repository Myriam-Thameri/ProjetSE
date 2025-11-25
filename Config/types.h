#ifndef TYPES_H

#define TYPES_H

typedef struct 
{
    int start_time; //After how much time from the beginning of the process
    int duration;
} IO_OPERATION;


typedef struct 
{
    char ID[4];
    int arrival_time;
    int execution_time;
    int priority;
    IO_OPERATION io_operations[20];
    int io_count;
} PROCESS;

#endif // TYPES_H