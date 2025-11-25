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


typedef struct {
    PROCESS process;
    int remaining_time;
    int executed_time;
    int io_index;
    int io_remaining;
    int finished;
    int in_io;
} PCB;

typedef struct QueueNode{
    PROCESS process;
    struct QueueNode* next;
} QueueNode;

typedef struct{
    QueueNode node;
    QueueNode* start;
    QueueNode* end;
    int size;

}QUEUE;

#endif // TYPES_H