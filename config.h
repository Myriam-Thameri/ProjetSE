#ifndef CONFIG_H
#define CONFIG_H

#define MAX_PROCESSES 128
#define MAX_IO_PER_PROCESS 50

typedef struct{
	int start_time;
	int duration;
} IO_OPERATION;

typedef struct{
	int id;
	int arrival_time;
	int execution_time;
	int priority;
	int io_count;
	IO_OPERATION io_operations[MAX_IO_PER_PROCESS];
} PROCESS;

int load_config(const char *filename, PROCESS p[], int *count);

#endif
