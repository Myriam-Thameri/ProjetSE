#ifndef FCFS_H
#define FCFS_H

#include "../Config/types.h"
#include "../Config/config.h"

/* Prototypes */
void FCFS_Algo(Config* config);

PCB* initialize_PCB(Config* config);
QUEUE add_process_to_queue(QUEUE ready_queue, PROCESS p);
QUEUE remove_process_from_queue(QUEUE ready_queue);

#endif // FCFS_H
