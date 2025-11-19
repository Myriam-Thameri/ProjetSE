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
	IO_OPERATION io_operations[50];
} PROCESS;
