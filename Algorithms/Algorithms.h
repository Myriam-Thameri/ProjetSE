#ifndef ALGORITHMS_H

#define ALGORITHMS_H

#include "../Config/types.h"

void RoundRobin_Algo(Config* config);

PCB* initialize_PCB(Config* config); //return an initialized PCB for each process in a list 

QUEUE add_process_to_queue(QUEUE ready_queue, PROCESS p); // add a node in the end of the ready queue and then return it 

QUEUE remove_process_from_queue(QUEUE ready_queue); //remove the first process in the queue and return the updated queue

void MultilevelAgingScheduler(Config* config);

#endif // ALGORITHMS_H