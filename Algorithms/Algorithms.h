#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "../Config/types.h"
#include "../Config/config.h"

/* Prototype du FCFS */
void FCFS_Algo(Config* config);

PCB* initialize_PCB(Config* config);
QUEUE add_process_to_queue(QUEUE ready_queue, PROCESS p);
QUEUE remove_process_from_queue(QUEUE ready_queue);

#endif // ALGORITHMS_H
